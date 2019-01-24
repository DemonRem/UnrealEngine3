using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.IO;
using System.Threading;
using System.Reflection;
using System.Xml;
using System.Xml.Serialization;
using System.Web;

namespace PIB.FileWebServices
{
	/// <summary>
	/// Manages our backend webservice interface
	/// </summary>
	public class GenericWebService : IDisposable
	{
		/// <summary>
		/// A blank result
		/// </summary>
		private byte[] NullResult = System.Text.Encoding.UTF8.GetBytes( "<?xml version=\"1.0\" encoding=\"utf-8\"?><NoResponse xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:nil=\"true\"/>" );
		
		/// <summary>
		/// Route requests for root (/) to this delegate 
		/// </summary>
        public string DefaultPath = "@list";

        /// <summary>
        /// Should we display exceptions to the user if they occur during execution?
        /// </summary>
        public bool DisplayExceptions = true;

        /// <summary>
        /// How many pending listeners to have (no need for a large number here)
        /// </summary>
        public int MaxSimultaniousListeners = 3;

		class Service
		{
			public Type request_type;
			public Type response_type;
			public MethodInfo handler;
			public string description;
		}

		class Content
		{
			public string TypeOfData;
			public byte[] BinaryData;
		}

        private LocalDataStoreSlot ServiceContext = Thread.AllocateDataSlot();
		private Dictionary<string, Dictionary<int, Service>> Services = new Dictionary<string, Dictionary<int, Service>>();
		private Dictionary<string, Content> StaticContent = new Dictionary<string, Content>();
		private HttpListener ServiceHttpListener = new HttpListener();
		private string mSubdir;
		private int mPort;
		private bool mShutdown = false;
        private string ServiceStartTime;

		/// <summary>
		/// ctor
		/// </summary>
		/// <param name="port">the port to run the webservice on</param>
		/// <param name="subdir">the subdirectory to claim</param>
		public GenericWebService( int port, string subdir )
		{
			mPort = port;
			mSubdir = subdir;
			ServiceStartTime = DateTime.UtcNow.ToString( "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'" );

			LoadStatic( "transform.xsl", "text/xsl" );
			LoadStatic( "logo.png", "image/png" );
			LoadStatic( "clientcode.js", "text/javascript" );
			LoadStatic( "style.css", "text/css" );

			ServiceHttpListener.Prefixes.Add( String.Format( "http://*:{0}/{1}/", mPort, mSubdir ) );
		}

		/// <summary>
		/// Start the HTTP listener
		/// </summary>
		public void Start()
		{
			ServiceHttpListener.Start();
			for( int i = 0; i < MaxSimultaniousListeners; ++i )
			{
				ServiceHttpListener.BeginGetContext( AsyncHandleHttpRequest, null );
			}
		}

		/// <summary>
		/// Stop the HTTP listener
		/// </summary>
		public void Stop()
		{
			ServiceHttpListener.Stop();
			mShutdown = true;
			ServiceHttpListener.Abort();
		}

		/// <summary>
		/// Dispose this class to stop the HTTP listener (calls Stop())
		/// </summary>
		public void Dispose()
		{
			Stop();
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="handler"></param>
        public bool AddServiceProvider<T>() where T : new()
		{
            // double check the new() constraint
            if( typeof( T ).GetConstructor( Type.EmptyTypes ) == null )
            {
                throw new ApplicationException( "Need to be able to create context types" );
            }

			// search all methods
            bool bAddedAnyServiceProviders = false;
            foreach( MethodInfo Method in typeof( T ).GetMethods() )
			{
				// further restrict to those with the WSA
				object[] WSAs = Method.GetCustomAttributes( typeof( WebServiceAttribute ), false );
				if( WSAs.Length <= 0 )
				{
					continue;
				}

				WebServiceAttribute WSA = ( WebServiceAttribute )WSAs[0];
				if( !WSA.IsValid )
				{
                    throw new ApplicationException( "WebServiceAttribute was not valid" );
				}

				// WSA functions must take a single parameter
				ParameterInfo[] ParmInfo = Method.GetParameters();
				if( Method.IsStatic || !Method.IsPublic )
				{
                    throw new ApplicationException( "WebServiceAttribute is only valid on Public, Non-Static methods." );
				}

				Type ReturnType = null;
				if( WSA.RawInput )
				{
					if( ParmInfo.Length != 3 || ParmInfo[0].ParameterType != typeof( string ) || ParmInfo[1].ParameterType != typeof( Encoding ) || ParmInfo[2].ParameterType != typeof( Stream ) )
					{
						throw new ApplicationException( "RawInput WebService functions take the form (string, Encoding, Stream)" );
					}
					ReturnType = null;
				}
				else
				{
					if( ParmInfo.Length != 1 )
					{
						throw new ApplicationException( "Non-RawInput WebService functions can accept only a single parameter" );
					}
					ReturnType = ParmInfo[0].ParameterType;
				}

                // create the service entry
                Service NewWebService = new Service();
				NewWebService.request_type = ReturnType;
				NewWebService.response_type = Method.ReturnType;
				NewWebService.handler = Method;
				NewWebService.description = WSA.Desc;

				// add it to our lookup
				Dictionary<int, Service> ServiceVersionDictionary;
				if( !Services.TryGetValue( WSA.Name, out ServiceVersionDictionary ) )
				{
					ServiceVersionDictionary = new Dictionary<int, Service>();
					Services.Add( WSA.Name, ServiceVersionDictionary );
				}
				ServiceVersionDictionary.Add( WSA.Version, NewWebService );

				// try to create the XmlSerializers now (so we know it's possible and we get errors on startup rather than during first exec)
				if( NewWebService.request_type != null )
                {
					new XmlSerializer( NewWebService.request_type );
                }

				if( NewWebService.response_type != typeof( void ) ) 
				{
					new XmlSerializer( NewWebService.response_type );
				}

                bAddedAnyServiceProviders = true;
			}

            return bAddedAnyServiceProviders;
		}

		/// <summary>
		/// Load a static element from a resource
		/// </summary>
		/// <param name="name">the name of the resource (should contain a .) (accessible at any path)</param>
		/// <param name="type">the MIME type to report</param>
		public void LoadStatic( string name, string type )
		{
			Assembly ExecutingAssembly = System.Reflection.Assembly.GetExecutingAssembly();
			string ResourceName = System.Reflection.MethodInfo.GetCurrentMethod().DeclaringType.Namespace + ".Resources." + name;
			Stream ResourceStream = ExecutingAssembly.GetManifestResourceStream( ResourceName );
			if( ResourceStream == null )
			{
				throw new Exception( "Could not load resource " + name );
			}

			Content ResourceContent = new Content();
			ResourceContent.TypeOfData = type;
			ResourceContent.BinaryData = new byte[ResourceStream.Length];
			ResourceStream.Read( ResourceContent.BinaryData, 0, ( int )ResourceStream.Length );
			StaticContent.Add( name, ResourceContent );
		}

		private object GetContextObject( Type t )
		{
			Dictionary<Type, object> map = ( Dictionary<Type, object> )Thread.GetData( ServiceContext );
			if( map == null )
			{
				map = new Dictionary<Type, object>();
				Thread.SetData( ServiceContext, map );
			}

			object obj;
			if( !map.TryGetValue( t, out obj ) )
			{
				obj = t.GetConstructor( Type.EmptyTypes ).Invoke( null );
				map.Add( t, obj );
			}
			return obj;
		}

		private void AsyncHandleHttpRequest( IAsyncResult Result )
		{
			try
			{
				if( mShutdown )
				{
					return;
				}

				HttpListenerContext Context = ServiceHttpListener.EndGetContext( Result );
				ServiceHttpListener.BeginGetContext( AsyncHandleHttpRequest, null );
				HttpListenerRequest Request = Context.Request;

				using( HttpListenerResponse Response = Context.Response )
				{
					string ErrorMessage = null;
					Stream OutputStream = Response.OutputStream;
					string[] UrlElements = Request.RawUrl.Split( new[] { '/' }, StringSplitOptions.RemoveEmptyEntries );

					// Handle listing the available methods (as not enough parameters found)
					if( UrlElements.Length < 2 )
					{
						// invalid subpath, redirect to our default
						if( DefaultPath != null )
						{
							Response.Redirect( String.Format( "/{0}/{1}", mSubdir, DefaultPath ) );
						}
						return;
					}

					// check for a hardcoded command
					if( UrlElements[1][0] == '@' )
					{
						if( UrlElements[1] == "@list" )
						{
							Response.SendChunked = true;
							Response.ContentType = "application/xml";
							using( StreamWriter sw = new StreamWriter( OutputStream, Encoding.UTF8 ) )
							{
								sw.Write( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
								sw.WriteLine( "<?xml-stylesheet type=\"text/xsl\" href=\"/{0}/transform.xsl\"?>\n", mSubdir );
								sw.WriteLine( "<ListMethods service=\"{0}\">\n", mSubdir );
								foreach( var key in Services.Keys )
								{
									Dictionary<int, Service> dict = Services[key];
									int ver = dict.Keys.Max();
									Service s = dict[ver];
									sw.WriteLine( "<Method name=\"{1}\" src=\"/{0}/{1}/{3}\">{2}</Method>", mSubdir, key, s.description.Replace( "<", "&lt;" ), ver );
								}
								sw.Write( "</ListMethods>\n" );
							}
						}
						else
						{
							Response.StatusCode = ( int )HttpStatusCode.BadRequest;
						}
						return;
					}

					// check for static content (available in any directory)
					if( StaticContent.ContainsKey( UrlElements[UrlElements.Length - 1] ) )
					{
						if( Request.Headers["If-Modified-Since"] == ServiceStartTime )
						{
							Response.StatusCode = ( int )HttpStatusCode.NotModified;
						}
						else
						{
							Content StaticContentItem = StaticContent[UrlElements[UrlElements.Length - 1]];
							Response.ContentType = StaticContentItem.TypeOfData;
							Response.ContentLength64 = StaticContentItem.BinaryData.LongLength;
							OutputStream.Write( StaticContentItem.BinaryData, 0, StaticContentItem.BinaryData.Length );
						}
						return;
					}

					// get configuration
					Dictionary<int, Service> ServiceVersionDictionary;
					if( Services.TryGetValue( UrlElements[1], out ServiceVersionDictionary ) )
					{
						int ServiceVersion = 0;
						if( UrlElements.Length > 2 )
						{
							Int32.TryParse( UrlElements[2], out ServiceVersion );
						}

						if( ServiceVersion <= 0 )
						{
							// redirect to the latest version
							Response.Redirect( String.Format( "/{0}/{1}/{2}", mSubdir, UrlElements[1], ServiceVersionDictionary.Keys.Max() ) );
							return;
						}

						// get the explicitly versioned service
						Service ServiceToRun;
						if( ServiceVersionDictionary.TryGetValue( ServiceVersion, out ServiceToRun ) )
						{
							// try to parse a request object (if any)
							object[] req = null;
							if( ServiceToRun.request_type == null )
							{
								// raw requests have the form (string type, Encoding encoding, Stream content)
								req = new object[] { Request.ContentType, Request.ContentEncoding, Request.InputStream };
							}
							else if( Request.HasEntityBody )
							{
								try
								{
									if( Request.ContentType.ToLower() == "application/x-www-form-urlencoded" )
									{
										// allow submission via HTML forms
										string post = new StreamReader( Request.InputStream, Encoding.UTF8 ).ReadToEnd();
										if( post.StartsWith( "xml=" ) )
										{
											post = HttpUtility.UrlDecode( post.Substring( 4 ) );
											req = new object[] { new XmlSerializer( ServiceToRun.request_type ).Deserialize( new StringReader( post ) ) };
										}
									}
									else
									{
										// try to deserialize as XML
										req = new object[] { new XmlSerializer( ServiceToRun.request_type ).Deserialize( Request.InputStream ) };
									}
								}
								catch( System.InvalidOperationException )
								{
									req = null;
								}
							}

							// call the handler for this path
							try
							{
								// get or create a context
								object obj = GetContextObject( ServiceToRun.handler.ReflectedType );

								// invoke the handler
								object resp = ServiceToRun.handler.Invoke( obj, req ?? new object[] { null } );
								// format the response
								Response.SendChunked = true;
								Response.ContentType = "application/xml";
								if( ServiceToRun.response_type != typeof( void ) )
								{
									// if there's a response send it
									using( XmlWriter wr = XmlWriter.Create( OutputStream ) )
									{
										wr.WriteProcessingInstruction( "xml-stylesheet", "type=\"text/xsl\" href=\"transform.xsl\"" );
										new XmlSerializer( ServiceToRun.response_type ).Serialize( wr, resp );
									}
								}
								else
								{
									OutputStream.Write( NullResult, 0, NullResult.Length );
								}

								// we're done as long as there were no exceptions
								return;
							}
							catch( System.Exception ex )
							{
								// display the exception (falls through)
								if( DisplayExceptions )
								{
									ErrorMessage = ex.ToString();
									ErrorMessage = ErrorMessage.Replace( "<", "&lt;" );
									ErrorMessage = ErrorMessage.Replace( "\n", "<br/>\n" );
								}
								else
								{
									ErrorMessage = "An unhandled exception occurred while executing this transaction.<br/>" + ex.Message;
								}

								Response.StatusCode = ( int )HttpStatusCode.InternalServerError;
							}
						}
					}

					// if we haven't returned by now, write out the error info
					if( Response.StatusCode == ( int )HttpStatusCode.OK )
					{
						Response.StatusCode = ( int )HttpStatusCode.NotFound;
					}

					Response.ContentType = "text/html";
					string ResponseString;
					if( Response.StatusCode == ( int )HttpStatusCode.NotFound )
					{
						ResponseString = String.Format( "<html><body><h1>Error 404<br/></h1>Could not find {0}</body></html>", Request.RawUrl );
					}
					else
					{
						ResponseString = String.Format( "<html><body><h1>Error {0}<br/></h1>{1}</body></html>", Response.StatusCode, ErrorMessage ?? "Unknown Error" );
					}

					byte[] buffer = System.Text.Encoding.UTF8.GetBytes( ResponseString );
					Response.ContentLength64 = buffer.Length;
					OutputStream.Write( buffer, 0, buffer.Length );
				}
			}
			catch( System.Exception )
			{
			}
		}
	}

	[AttributeUsage( AttributeTargets.Method )]
	public class WebServiceAttribute : System.Attribute
	{
		public string Name { get; set; }
		public int Version { get; set; }
		public string Desc { get; set; }
        public bool RawInput { get; set; }

		public bool IsValid
		{
            get { return Name != null && Name != "" && Version > 0 && !Name.Contains('/'); }
		}

        public WebServiceAttribute() 
            : this("", 1) 
        {
        }

        public WebServiceAttribute(string name, int version)
        {
            Name = name;
            Version = version;
            RawInput = false;
        }
	}
}
