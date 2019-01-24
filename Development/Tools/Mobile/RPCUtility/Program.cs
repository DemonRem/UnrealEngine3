using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Reflection;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Ipc;
using System.Runtime.Remoting.Channels.Tcp;
using System.Runtime.Remoting.Lifetime;
using System.Threading;

namespace RPCUtility
{
	public class RPCUtility : MarshalByRefObject
	{
		public Int32 MaximumConcurrentCommands = Environment.ProcessorCount;
		public Int32 RequestConcurrentCommands = 0;
		public Int32 CurrentConcurrentCommands = 0;
		public Semaphore ConcurrentCommandControl;
		public Object ConcurrentCommandControlLock = new Object();

		public override Object InitializeLifetimeService()
		{
			// We don't ever want this to expire...
			ILease NewLease = ( ILease )base.InitializeLifetimeService();
			if( NewLease.CurrentState == LeaseState.Initial )
			{
				NewLease.InitialLeaseTime = TimeSpan.Zero;
			}
			return NewLease;
		}

		static public Int32 QueryTotalSystemMemoryMB = 0;
		static public Int32 GetMBFromString( string ValueString )
		{
			Int32 TestValue = 0;
			Int32 FinalValue = 0;
			if( ValueString.EndsWith( "G" ) )
			{
				if( Int32.TryParse( ValueString.Replace( "G", "" ), out TestValue ) )
				{
					FinalValue = TestValue * 1024;
				}
			}
			else if( ValueString.EndsWith( "M" ) )
			{
				if( Int32.TryParse( ValueString.Replace( "M", "" ), out TestValue ) )
				{
					FinalValue = TestValue;
				}
			}
			return FinalValue;
		}
		static public void OutputReceivedForSystemInfoQuery( Object Sender, DataReceivedEventArgs Line )
		{
			if( ( Line != null ) && ( Line.Data != null ) && ( Line.Data != "" ) && ( Line.Data.StartsWith( "PhysMem" ) ) )
			{
				Debug.WriteLine( "    Got: " + Line.Data );

				string RawOutput = Line.Data;
				string[] ParsedRawOutput = RawOutput.Split( ' ' );

				// Expect output like this:
				// PhysMem: 1871M wired, 1084M active, 6735M inactive, 9689M used, 15G free.

				Int32 InactiveMemory = GetMBFromString( ParsedRawOutput[5] );
				Int32 FreeMemory = GetMBFromString( ParsedRawOutput[9] );
				QueryTotalSystemMemoryMB = InactiveMemory + FreeMemory;
			}
		}
		public void UpdateMaximumConcurrentCommands()
		{
			Console.WriteLine( "  Begin updating the number of concurrent commands supported" );

			Int32 NewMaximumConcurrentCommands = MaximumConcurrentCommands;
			try
			{
				// Attempt a couple different way to get the system memory
				PerformanceCounter TotalSystemMemoryCounter = new PerformanceCounter( "Memory", "Available Mbytes" );
				Int32 TotalSystemMemoryMB = (Int32)TotalSystemMemoryCounter.NextValue();
				if( TotalSystemMemoryMB == 0 )
				{
					// That technique didn't work, we might be on non-Windows.
					// Try shelling out to top for detailed information.
					Process QueryProcess = new Process();
					QueryProcess.StartInfo.FileName = "/usr/bin/top";
					QueryProcess.StartInfo.Arguments = string.Format( "-l 1" );
					QueryProcess.StartInfo.UseShellExecute = false;
					QueryProcess.StartInfo.RedirectStandardOutput = true;
					QueryProcess.StartInfo.RedirectStandardError = true;
					QueryProcess.OutputDataReceived += new DataReceivedEventHandler( OutputReceivedForSystemInfoQuery );
					QueryProcess.ErrorDataReceived += new DataReceivedEventHandler( OutputReceivedForSystemInfoQuery );

					// Try to launch the query's process, and produce a friendly error message if it fails.
					try
					{
						// Start the process up and then wait for it to finish
						if( QueryProcess.Start() )
						{
							QueryProcess.BeginOutputReadLine();
							QueryProcess.BeginErrorReadLine();
							QueryProcess.WaitForExit();
						}
					}
					catch( Exception )
					{
						string Message = String.Format( "Failed to start local process for action: {0} {1}", QueryProcess.StartInfo.FileName, QueryProcess.StartInfo.Arguments );
						Console.WriteLine( Message );
						throw new Exception( Message );
					}
					if( QueryTotalSystemMemoryMB != 0 )
					{
						Debug.WriteLine( "    Got available system memory from a system call to /usr/bin/top" );
						TotalSystemMemoryMB = QueryTotalSystemMemoryMB;
					}
				}
				else
				{
					Debug.WriteLine( "    Got available system memory from a system performance counter" );
				}

				if( TotalSystemMemoryMB != 0 )
				{
					// Divide by the magic number (which is roughly the maximum MBs a (compile) task is expected to use)
					NewMaximumConcurrentCommands = Math.Max( TotalSystemMemoryMB / 600, 1 );
				}
			}
			catch
			{
				Console.WriteLine( "Failed to get the available system memory amount" );
			}

			// If we're initializing for the first time, or if the value has changed, update
			if( ConcurrentCommandControl == null ||
				NewMaximumConcurrentCommands != MaximumConcurrentCommands )
			{
				MaximumConcurrentCommands = NewMaximumConcurrentCommands;
				Console.WriteLine( " -> Updating with support for {0} concurrent commands", MaximumConcurrentCommands );
				ConcurrentCommandControl = new Semaphore(
					MaximumConcurrentCommands,
					MaximumConcurrentCommands
				);
			}
			else
			{
				Console.WriteLine( "    Number of concurrent commands remains unchanged" );
			}
			Console.WriteLine( "  Done updating the number of concurrent commands supported" );
		}

		public bool Maintain()
		{
			if( Console.KeyAvailable )
			{
				return false;
			}
			return true;
		}

		public class ProcessWrapper
		{
			public Process CommandProcess;
			public string  CommandOutput;

			public void OutputReceivedDataEventHandler( Object Sender, DataReceivedEventArgs Line )
			{
				if( ( Line != null ) && ( Line.Data != null ) )
				{
					Debug.WriteLine( "    Output : " + Line.Data );
					CommandOutput += Line.Data;
					CommandOutput += Environment.NewLine;
				}
			}
		}

		public Hashtable ExecuteCommand( Hashtable CommandDescription )
		{
			// Extract the version information and pass the command along
			Version CommandVersion = CommandDescription["Version"] as Version;
			Hashtable ReturnValues = null;

			// Before we execute any commands, update the maximum number of concurrent tasks
			lock( ConcurrentCommandControlLock )
			{
				if( RequestConcurrentCommands == 0 )
				{
					UpdateMaximumConcurrentCommands();
				}
				RequestConcurrentCommands++;
			}

			// We only care about Major and Minor revisions
			switch( CommandVersion.Major )
			{
				case 1:
					switch( CommandVersion.Minor )
					{
						case 0:
							ReturnValues = ExecuteCommand_1_0( CommandDescription );
						break;
					}
				break;
			}

			// Update the number of active commands
			lock( ConcurrentCommandControlLock )
			{
				RequestConcurrentCommands--;
			}

			return ReturnValues;
		}

		public Hashtable ExecuteCommand_1_0( Hashtable CommandDescription )
		{
			// Extract parameters
			string CommandName = CommandDescription["CommandName"] as string;
			string CommandArgs = CommandDescription["CommandArgs"] as string;
			string WorkingDirectory = CommandDescription["WorkingDirectory"] as string;

			Debug.WriteLine( "Executing..." );
			Debug.WriteLine( "    Working directory : " + WorkingDirectory );
			Debug.WriteLine( "    Command name      : " + CommandName );
			Debug.WriteLine( "    Command args      : " + CommandArgs );

			Hashtable ReturnValues = new Hashtable();

			// Short circuit for special case commands
			if( CommandName.StartsWith( "rpc:" ) )
			{
				switch( CommandName )
				{
					case "rpc:command_slots_available":
						// Be sure to add one to account for *this* task
						ReturnValues["CommandOutput"] = ( MaximumConcurrentCommands - RequestConcurrentCommands + 1 ).ToString();
					break;
				}
			}
			else
			{
				// The general case of an arbitrary command
				ProcessWrapper CommandProcessWrapper = new ProcessWrapper();
				CommandProcessWrapper.CommandProcess = new Process();

				CommandProcessWrapper.CommandProcess.StartInfo.FileName = CommandName;
				CommandProcessWrapper.CommandProcess.StartInfo.Arguments = CommandArgs;
				CommandProcessWrapper.CommandProcess.StartInfo.WorkingDirectory = WorkingDirectory;
				CommandProcessWrapper.CommandProcess.StartInfo.CreateNoWindow = true;

				CommandProcessWrapper.CommandProcess.StartInfo.UseShellExecute = false;
				CommandProcessWrapper.CommandProcess.StartInfo.RedirectStandardOutput = true;
				CommandProcessWrapper.CommandProcess.StartInfo.RedirectStandardError = true;
				CommandProcessWrapper.CommandProcess.StartInfo.RedirectStandardInput = true;
				CommandProcessWrapper.CommandProcess.OutputDataReceived += new DataReceivedEventHandler( CommandProcessWrapper.OutputReceivedDataEventHandler );
				CommandProcessWrapper.CommandProcess.ErrorDataReceived += new DataReceivedEventHandler( CommandProcessWrapper.OutputReceivedDataEventHandler );

				// Try to launch the action's process, and produce a friendly error message if it fails
				try
				{
					// Wait until there's a free slot to execute in
					ConcurrentCommandControl.WaitOne();
					lock( ConcurrentCommandControlLock )
					{
						CurrentConcurrentCommands++;
						Console.WriteLine( "     + {0} of {1} command slots taken, {2} requested", CurrentConcurrentCommands, MaximumConcurrentCommands, RequestConcurrentCommands );
					}

					// Start the process up and then wait for it to finish
					Int32 RetryCount = 0;
					Int32 MaxRetryCount = 2;
					bool CommandExecutionSuccess = false;
					do 
					{
						try
						{
							if( CommandProcessWrapper.CommandProcess.Start() )
							{
								CommandProcessWrapper.CommandProcess.BeginOutputReadLine();
								CommandProcessWrapper.CommandProcess.BeginErrorReadLine();
								CommandProcessWrapper.CommandProcess.WaitForExit();

								// Collect output once the application is finished
								ReturnValues["CommandOutput"] = CommandProcessWrapper.CommandOutput;
								ReturnValues["ExitCode"] = CommandProcessWrapper.CommandProcess.ExitCode;
								CommandExecutionSuccess = true;
							}
							else
							{
								ReturnValues["CommandOutput"] = "An error has occurred!";
								ReturnValues["ExitCode"] = -1;
							}
						}
						catch( Exception InnerEx )
						{
							// Look for a specific set of error cases that we want to work around
							if( ( InnerEx.ToString().Contains( "System.IO.IOException: Error creating standard error pipe" ) ||
								  InnerEx.ToString().Contains( "System.IO.IOException: Error creating standard output pipe" ) ) &&
								RetryCount < MaxRetryCount )
							{
								// Sleep for a bit before we try again
								RetryCount++;
								Thread.Sleep( 2000 );
							}
							else
							{
								// All out of retries, just re-throw the exception
								throw InnerEx;
							}
						}
					}
					while( (CommandExecutionSuccess == false) && (RetryCount < MaxRetryCount) );
				}
				catch( Exception Ex )
				{
					ReturnValues["CommandOutput"] = "An error has occurred! " + Ex.ToString();
					ReturnValues["ExitCode"] = -1;
				}
				finally
				{
					// Release the current command slot
					ConcurrentCommandControl.Release();
					lock( ConcurrentCommandControlLock )
					{
						CurrentConcurrentCommands--;
						Console.WriteLine( "     - {0} of {1} command slots taken, {2} requested", CurrentConcurrentCommands, MaximumConcurrentCommands, RequestConcurrentCommands );
					}
				}
			}

			Debug.WriteLine( "Command execution complete" );

			return ReturnValues;
		}
	}

	public class RPCUtilityApplication
	{
		/**
		 * Utility for simply pinging a remote host to see if it's alive.
		 * Only returns true if the host is pingable, false for all
		 * other cases.
		 */
		static bool PingRemoteHost( string RemoteHostNameOrAddressToPing )
		{
			bool PingSuccess = false;
			try
			{
				// The default Send method will timeout after 5 seconds
				Ping PingSender = new Ping();
				PingReply Reply = PingSender.Send( RemoteHostNameOrAddressToPing );
				if( Reply.Status == IPStatus.Success )
				{
					Debug.WriteLine( "Successfully pinged " + RemoteHostNameOrAddressToPing );
					PingSuccess = true;
				}
			}
			catch( Exception Ex )
			{
				// This will catch any error conditions like unknown hosts
				Console.WriteLine( "Failed to ping " + RemoteHostNameOrAddressToPing );
				Console.WriteLine( "Exception details: " + Ex.ToString() );
			}
			return PingSuccess;
		}

		[STAThread]
		static Int32 Main( string[] args )
		{
			Int32 ExitCode = 0;

			// No parameters means run as a server
			if( args.Length == 0 )
			{
				Console.WriteLine( "Server starting up" );
				try
				{
					RemotingConfiguration.RegisterWellKnownServiceType( typeof( RPCUtility ), "RPCUtility", WellKnownObjectMode.Singleton );
					Console.WriteLine( "Server type registered with remoting system" );

					// allow settings to control what IP address to bind to
					string HostIP = null;
					if (!String.IsNullOrEmpty(Properties.Settings.Default.OverrideHostAddr))
					{
						HostIP = Properties.Settings.Default.OverrideHostAddr;
					}
					else
					{
						// get the IP address of the local machine (allow the settings to override
						// the machine name, as it appears a Mac on a Windows domain will get a funky
						// machine name, like sep00391a3dec, instead of what is posted to DNS) 

						// default to what the OS tells us
						string HostName = Dns.GetHostName();
						// allow for overide
						if (!String.IsNullOrEmpty(Properties.Settings.Default.OverrideHostName))
						{
							HostName = Properties.Settings.Default.OverrideHostName;
						}

						// report what we are using
						Console.WriteLine("Server is using host name {0}", HostName);

						try
						{
							IPAddress[] Addresses = Dns.GetHostAddresses(HostName);
							// look for an IP4 IP address
							foreach (IPAddress IP in Addresses)
							{
								if (IP.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
								{
									HostIP = IP.ToString();
								}
							}
						}
						catch (System.Exception)
						{
							Console.WriteLine("\n\nDNS lookup failed for {0}!\nYou will need to edit your RPCUtility.exe.config to override the HostName or HostAddr.\n" + 
								"If your Mac is on a Windows domain, set OverrideHostName to the machine name you use to connect to the Mac from your PC:\n\n" + 
								"  <setting name=\"OverrideHostname\" serializeAs=\"String\">\n" +
								"    <value>MyMac</value>\n" + 
								"  </setting>\n",
								HostName);
							ExitCode = -3;
						}
					}

					if (!String.IsNullOrEmpty(HostIP))
					{
						// Specify the properties for the server channel.
						System.Collections.IDictionary Settings = new System.Collections.Hashtable();
						Settings["port"] = Properties.Settings.Default.RPCPortName;
						Settings["bindTo"] = HostIP;

						Console.WriteLine("Server is using host IP addr {0}", HostIP);

						TcpChannel ServerChannel = new TcpChannel(Settings, null, null);
						ChannelServices.RegisterChannel(ServerChannel, false);
						Console.WriteLine("Server channel registered");

						try
						{
							string ServerURL = String.Format("tcp://{0}:{1}/RPCUtility", HostIP, Properties.Settings.Default.RPCPortName);
							RPCUtility ServerRPCUtility = (RPCUtility)Activator.GetObject(typeof(RPCUtility), ServerURL);
							Console.WriteLine("Server object created");

							Console.WriteLine("Server running, press any key to exit...");
							while (ServerRPCUtility.Maintain())
							{
								// Running
								Thread.Sleep(1000);
							}
							Console.WriteLine("Server shutting down");
						}
						catch (Exception Ex2)
						{
							Console.WriteLine("Inner Error: " + Ex2.ToString());
							ExitCode = -2;
						}

						ChannelServices.UnregisterChannel(ServerChannel);
						Console.WriteLine("Server channel unregistered");
					}
				}
				catch( Exception Ex )
				{
					Console.WriteLine( "Outer Error: " + Ex.ToString() );
					ExitCode = -1;
				}

			}
			else
			{
				Debug.WriteLine( "Client starting up" );
				try
				{
					// Extract the command line parameters
					if( args.Length < 2 )
					{
						throw new Exception( "Too few arguments! Usage: Utility.exe MachineName CommandName [CommandArg...]" );
					}
					string MachineName = args[0];
					string WorkingDirectory = args[1];
					string CommandName = args[2];
					string CommandArgs = "";
					if( args.Length > 3 )
					{
						CommandArgs = String.Join( " ", args, 3, args.Length - 3 );
					}

					Debug.WriteLine( "    Machine name      : " + MachineName );
					Debug.WriteLine( "    Working directory : " + WorkingDirectory );
					Debug.WriteLine( "    Command name      : " + CommandName );
					Debug.WriteLine( "    Command args      : " + CommandArgs );

					// Make sure we can ping the remote machine before we continue
					if( PingRemoteHost( MachineName ) )
					{
						RemotingConfiguration.RegisterWellKnownServiceType( typeof( RPCUtility ), "RPCUtility", WellKnownObjectMode.Singleton );
						Debug.WriteLine( "Server type registered with remoting system" );

						TcpChannel ClientChannel = new TcpChannel();
						ChannelServices.RegisterChannel( ClientChannel, false );
						Debug.WriteLine( "Client channel registered" );
						try
						{
							string RemoteRPCUtilityURL = String.Format( "tcp://{0}:{1}/RPCUtility", MachineName, Properties.Settings.Default.RPCPortName );
							RPCUtility RemoteRPCUtility = ( RPCUtility )Activator.GetObject( typeof( RPCUtility ), RemoteRPCUtilityURL );
							Debug.WriteLine( "Client object created" );

							// Do the work!
							Hashtable CommandParameters = new Hashtable();
							CommandParameters["Version"] = new Version( 1, 0 );
							CommandParameters["WorkingDirectory"] = WorkingDirectory;
							CommandParameters["CommandName"] = CommandName;
							CommandParameters["CommandArgs"] = CommandArgs;

							Random Generator = new Random();
							Int32 RetriesRemaining = 1;
							while( RetriesRemaining >= 0 )
							{
								try
								{
									// Try to execute the command
									Hashtable CommandResult = RemoteRPCUtility.ExecuteCommand( CommandParameters );

									if( CommandResult["ExitCode"] != null )
									{
										ExitCode = ( Int32 )CommandResult["ExitCode"];
									}
									Console.WriteLine( CommandResult["CommandOutput"] as string );
									break;
								}
								catch( Exception Ex3 )
								{
									if( RetriesRemaining > 0 )
									{
										// Generate a random retry timeout of 3-5 seconds
										Int32 RetryTimeoutMS = 3000 + (Int32)( 2000 * Generator.NextDouble() );
										Console.WriteLine( "Retrying command after sleeping for " + RetryTimeoutMS + " milliseconds. Command is:" + CommandName + " " + CommandArgs );
										Thread.Sleep( RetryTimeoutMS );
									}
									else
									{
										// We've tried enough times, just throw the error
										throw new Exception( "Deep Error, retries exhausted... ", Ex3 );
									}
									RetriesRemaining--;
								}
							}
						}
						catch( Exception Ex2 )
						{
							Console.WriteLine( "Inner Error: " + Ex2.ToString() );
							ExitCode = -2;
						}
						ChannelServices.UnregisterChannel( ClientChannel );
						Debug.WriteLine( "Client channel unregistered" );
					}
					else
					{
						Console.WriteLine( "Failed to ping remote host: " + MachineName );
						ExitCode = -3;
					}
				}
				catch( Exception Ex )
				{
					Console.WriteLine( "Outer Error: " + Ex.ToString() );
					ExitCode = -1;
				}

				Debug.WriteLine( "Client shutting down" );
			}

			return ExitCode;
		}
	}
}
