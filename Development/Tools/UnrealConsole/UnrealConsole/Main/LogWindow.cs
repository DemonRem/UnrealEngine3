
using System;
using System.Drawing;
using System.Windows.Forms;
using System.Collections;
using System.Runtime.InteropServices;



namespace UnrealConsole
{
	/// <summary>
	/// Summary description for LogWindow.
	/// </summary>
	public class FLogWindow
	{
		#region Win32 glue

		// import the SendMessage function so we can send windows messages to the control
		[DllImport("user32", CharSet = CharSet.Auto)]
		private static extern int SendMessage(HandleRef hWnd, int msg, int wParam, ref POINT lp);

		[DllImport("user32", CharSet = CharSet.Auto)]
		private static extern int SendMessage(HandleRef hWnd, int msg, int wParam, int lParam);

		// Constants from the Platform SDK.
		private const int EM_SETEVENTMASK = 1073;
		private const int EM_GETSCROLLPOS = 0x0400 + 221;
		private const int EM_SETSCROLLPOS = 0x0400 + 222;
		private const int WM_SETREDRAW = 11;

		[StructLayout(LayoutKind.Sequential)]
		private struct POINT
		{
			public long X;
			public long Y;
		}

		#endregion

		public FLogWindow(RichTextBox TTYOutput, TextBox TTYInput)
		{
			this.TTYOutput = TTYOutput;
			this.TTYInput = TTYInput;
			CommandHistory[0] = "";
			NumCommands = 1;
		}

		private Point ZeroPoint = new System.Drawing.Point(0, 0);
		public void Print( string Text )
		{
			OutputDebugString( Text );

			// if the end of the buffer is visible, keep in sync
			if (TTYOutput.DisplayRectangle.Contains(TTYOutput.GetPositionFromCharIndex(TTYOutput.TextLength)))
			{
				TTYOutput.AppendText(Text);
				return;
			}

			// rmember our scrolled location
			int OldSelectionStart = TTYOutput.SelectionStart;
			int OldSelectionLength = TTYOutput.SelectionLength;

			// prevent updates while we muck with the text
			int OldEventMask = SendMessage(new HandleRef(TTYOutput, TTYOutput.Handle), EM_SETEVENTMASK, 0, 0);
			SendMessage(new HandleRef(TTYOutput, TTYOutput.Handle), WM_SETREDRAW, 0, 0);

			// store the current scrollposition
			POINT ScrollPos = new POINT();
			SendMessage(new HandleRef(TTYOutput, TTYOutput.Handle), EM_GETSCROLLPOS, 0, ref ScrollPos);

			TTYOutput.AppendText( Text );

			// restore selection 
			TTYOutput.SelectionStart = OldSelectionStart;
			TTYOutput.SelectionLength = OldSelectionLength;

			// let the window update again
			SendMessage(new HandleRef(TTYOutput, TTYOutput.Handle), WM_SETREDRAW, 1, 0);
			SendMessage(new HandleRef(TTYOutput, TTYOutput.Handle), EM_SETEVENTMASK, 0, OldEventMask);

			// restore the scrollposition
			SendMessage(new HandleRef(TTYOutput, TTYOutput.Handle), EM_SETSCROLLPOS, 0, ref ScrollPos);

			TTYOutput.Refresh();
		}

		public void ClearOutput( )
		{
			TTYOutput.Clear();
		}

		public void ClearInput( )
		{
			bUserEnteringText = false;
			TTYInput.Clear();
		}

		public void SetInput( string Text )
		{
			bUserEnteringText = false;
			TTYInput.Text = Text;
		}

        /// <summary>
        /// Get text over a debugger connection from target
        /// </summary>
        public void ReceiveOutputTextFromTarget()
        {
            // @todo: Have a connected target variable
			string Text = UnrealConsole.DLLInterface.ReceiveOutputText(UnrealConsole.ConnectedTargetIndex);
            if (Text != null)
            {
                Print(Text);
            }

			if (UnrealConsole.DLLInterface.GetConsoleState(UnrealConsole.ConnectedTargetIndex) == (int)EConsoleState.CS_Crashed)
			{
				Print("\n\n\nDetected a crash:\n");
				ArrayList Callstack = new ArrayList();
				// get the callstack from the target
				if (UnrealConsole.DLLInterface.GetCallstack(UnrealConsole.ConnectedTargetIndex, Callstack))
				{
					// load symbols for the running executable
					if (UnrealConsole.DLLInterface.LoadSymbols(UnrealConsole.ConnectedTargetIndex, null))
					{
						// go voer it, and convert the callstack to readable addresses
						foreach (UInt32 Address in Callstack)
						{
							Print(UnrealConsole.DLLInterface.ResolveAddressToString(Address));
							Print("\n");
						}

						UnrealConsole.DLLInterface.UnloadAllSymbols();
						Print("\n\n\n");
					}
				}
			}
        }

		public string ProcessKeyInput( KeyPressEventArgs KeyEvent )
		{
			switch ( KeyEvent.KeyChar )
			{
				case (char)13:
					KeyEvent.Handled = true;
					if ( TTYInput.Text.Length > 0 )
					{
						Print( "> " + TTYInput.Text + "\r\n" );
						int PrevCommand = ( CurrentCommand == 0 ) ? (NumCommands-1) : (CurrentCommand-1);
						if ( NumCommands == 1 || TTYInput.Text != CommandHistory[ PrevCommand ] )
						{
							// Make sure it's stored completely (should already be, though)
							CommandHistory[ CurrentCommand ] = TTYInput.Text;
							if ( NumCommands < MaxCommands )
							{
								CurrentCommand = NumCommands++;
							}
							else
							{
								CurrentCommand = (CurrentCommand + 1) % MaxCommands;
							}
							PrevCommand = ( CurrentCommand == 0 ) ? (NumCommands-1) : (CurrentCommand-1);
						}

						// Clear the new command
						CommandHistory[CurrentCommand] = "";

						// Point History Index to the current
						CurrentHistoryIndex = CurrentCommand;

						// @todo: Have a target variable
                        UnrealConsole.DLLInterface.SendConsoleCommand(UnrealConsole.ConnectedTargetIndex, TTYInput.Text);

						ClearInput();
						return CommandHistory[ PrevCommand ];
					}
					break;
			}
			return null;
		}

		public void ProcessInputChanged( )
		{
			if ( bUserEnteringText )
			{
				CommandHistory[CurrentCommand] = TTYInput.Text;
			}
			bUserEnteringText = true;
		}

		public void ProcessKeyDown( KeyEventArgs KeyEvent )
		{
			switch ( KeyEvent.KeyCode )
			{
				case Keys.Up:
				{
					int NewCurrentHistoryIndex = ( CurrentHistoryIndex == 0 ) ? (NumCommands-1) : (CurrentHistoryIndex-1);
					if ( NumCommands > 0 && NewCurrentHistoryIndex != CurrentCommand )
					{
						CurrentHistoryIndex = NewCurrentHistoryIndex;
						SetInput( CommandHistory[ CurrentHistoryIndex ] );
					}
					KeyEvent.Handled = true;
					break;
				}

				case Keys.Down:
				{
					if ( NumCommands > 0 && CurrentHistoryIndex != CurrentCommand )
					{
						CurrentHistoryIndex = (CurrentHistoryIndex + 1) % NumCommands;
						SetInput( CommandHistory[ CurrentHistoryIndex ] );
					}
					KeyEvent.Handled = true;
					break;
				}
			}
		}

		public void OutputDebugString( string Text )
		{
			System.Diagnostics.Debug.Write( Text );
		}

		/// <summary>
		/// Toggle word wrapping in the TTY output window
		/// </summary>
		/// <returns>true if word wrap is enabled</returns>
		public bool ToggleWordWrap()
		{
			TTYOutput.WordWrap = !TTYOutput.WordWrap;
			TTYOutput.Refresh();

			return TTYOutput.WordWrap;
		}

		private RichTextBox	TTYOutput;
		private TextBox		TTYInput;
		private const int	MaxCommands = 256;
		private string[]	CommandHistory = new string[MaxCommands];
		private int			NumCommands = 0;
		private int			CurrentCommand = 0;
		private int			CurrentHistoryIndex = 0;
		private bool		bUserEnteringText = true;
	}
}
