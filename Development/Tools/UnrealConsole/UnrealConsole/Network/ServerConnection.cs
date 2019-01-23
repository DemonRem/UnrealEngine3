
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace UnrealNetwork
{
	/// <summary>
	/// Maintains a connection to a server. Holds both the send and receive 
	/// connections to the server
	/// </summary>
	public class ServerConnection
	{
		/// <summary>
		/// The name of the server we are communicating with
		/// </summary>
		private string	ServerName;
		/// <summary>
		/// What platform the server is running on
		/// </summary>
		private PlatformType ServerPlatform;
		/// <summary>
		/// The IP address of the server we are communicating with
		/// </summary>
		private IPAddress ServerAddress;
		/// <summary>
		/// The port to send server requests to
		/// </summary>
		private int RequestPortNo;
		/// <summary>
		/// The port to listen for server responses on
		/// </summary>
		private int ListenPortNo;
		/// <summary>
		/// Used to receive data from the remote server
		/// </summary>
		private AsyncUdpClient ListenClient;
		/// <summary>
		/// Used to send commands to the remote server
		/// </summary>
		private UdpClient RequestClient;

		/// <summary>
		/// Copies the specified values into the appropriate members. Does
		/// not create the underlying socket connections. That is done in
		/// Connect(). Use Disconnect() to stop all socket processing.
		/// </summary>
		/// <param name="InServerName">The name of the server we'll be communicating with</param>
		/// <param name="InPlatform">What platform the server is running on</param>
		/// <param name="InAddress">The address we'll be communicating with</param>
		/// <param name="InRequestPortNo">The port to send server requests to</param>
		/// <param name="InListenPortNo">The port to listen for server responses on</param>
		public ServerConnection(string InServerName, PlatformType InPlatform, IPAddress InAddress,int InRequestPortNo,
			int InListenPortNo)
		{
			ServerName = InServerName;
			ServerPlatform = InPlatform;
			ServerAddress = InAddress;
			RequestPortNo = InRequestPortNo;
			ListenPortNo = InListenPortNo;
		}

		/// <summary>
		/// Connects the sockets to their corresponing ports
		/// </summary>
		public void Connect()
		{
			RequestClient = new UdpClient();
			// Connect to the server's listener
			RequestClient.Connect(ServerAddress,RequestPortNo);
			// Create our connection that we'll read from
			ListenClient = new AsyncUdpClient(ListenPortNo);
			ListenClient.StartReceiving();
		}

		/// <summary>
		/// Closes both of the connections
		/// </summary>
		public void Disconnect()
		{
			RequestClient.Close();
			ListenClient.StopReceiving();
		}

		/// <summary>
		/// Sends a client connect request to the server
		/// </summary>
		public void SendConnectRequest()
		{
			// Send the 'CC' client connect request
			Byte[] BytesToSend = Encoding.ASCII.GetBytes("CC");
			RequestClient.Send(BytesToSend,BytesToSend.Length);
		}

		/// <summary>
		/// Sends a client message to the server
		/// </summary>
		public void SendCommand( string Message )
		{
			int Length = Message.Length;
			Byte[] TagBytes = Encoding.ASCII.GetBytes("CT");
			Byte[] AsciiBytes = Encoding.ASCII.GetBytes(Message);
			Byte[] BytesToSend = new Byte[ Length + 6 ];
			BytesToSend[0] = TagBytes[0];
			BytesToSend[1] = TagBytes[1];
			BytesToSend[2] = (Byte) ((Length & 0xff000000) >> 24);
			BytesToSend[3] = (Byte) ((Length & 0x00ff0000) >> 16);
			BytesToSend[4] = (Byte) ((Length & 0x0000ff00) >> 8);
			BytesToSend[5] = (Byte) ((Length & 0x000000ff) >> 0);
			for ( int Index=0; Index < Message.Length; ++Index )
			{
				BytesToSend[Index+6] = AsciiBytes[Index];
			}
			RequestClient.Send(BytesToSend,BytesToSend.Length);
		}

		/// <summary>
		/// Sends a client disconnect request to the server
		/// </summary>
		public void SendDisconnectRequest()
		{
			// Send the 'CD' client disconnect request
			Byte[] BytesToSend = Encoding.ASCII.GetBytes("CD");
			RequestClient.Send(BytesToSend,BytesToSend.Length);
		}

		/// <summary>
		/// Gets the next packet waiting in the async queue
		/// </summary>
		/// <returns>The next available packet or null if none are waiting</returns>
		public Packet GetNextPacket()
		{
			return ListenClient.GetNextPacket();
		}

		/// <summary>
		/// Returns the address of the server that we are connected to
		/// </summary>
		public string Name
		{
			get
			{
				return ServerName;
			}
		}

		/// <summary>
		/// Returns the address of the server that we are connected to
		/// </summary>
		public IPAddress Address
		{
			get
			{
				return ServerAddress;
			}
		}

		/// <summary>
		/// Returns the port of the server that we are connected to
		/// </summary>
		public int Port
		{
			get
			{
				return ListenPortNo;
			}
		}

		/// <summary>
		/// Returns the platform the server is running on
		/// </summary>
		public PlatformType Platform
		{
			get
			{
				return ServerPlatform;
			}
		}
	}
}
