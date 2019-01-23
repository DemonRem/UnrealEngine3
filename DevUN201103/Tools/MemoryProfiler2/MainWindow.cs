/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

using EnvDTE;
using EpicCommonUtils;

namespace MemoryProfiler2
{
	public partial class MainWindow : Form
	{
		/** A class that is serialised to disk with settings info */
		public SettableOptions Options = null;

		/** List of snapshots created by parser and used by various visualizers and dumping tools. */
		private List<FStreamSnapshot> SnapshotList;
		/** Current active snapshot. Either a diff if start is != start of actual snapshot if start == start. */
		private FStreamSnapshot CurrentSnapshot;
		/** Current file name associated with open file. */
		public string CurrentFilename;
		/** Parent address to use when creating call graph. */
		private int ParentFunctionIndex = -1;

        /** Progress indicator window */
        SlowProgressDialog ProgressDialog;

        private void CommonInit()
        {
			Options = ReadOptions<SettableOptions>( Path.Combine( Application.StartupPath, "MemoryProfiler2.ClassGroups.xml" ) );

            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler( CurrentDomain_AssemblyResolve );

            ExclusiveListView.ListViewItemSorter = new ColumnSorter();

			PopulateClassGroups();

            ProgressDialog = new SlowProgressDialog();
        }

		public MainWindow()
		{
			InitializeComponent();
        
            CommonInit();
        }

		public MainWindow( string Filename)
		{
			InitializeComponent();

            CommonInit();

			ParseFile(Filename);
		}

		/** 
		 * Set the wait cursor to the hourglass for long operations
		 */
		private void SetWaitCursor( bool bOn )
		{
			UseWaitCursor = bOn;
			if( UseWaitCursor )
			{
				Cursor.Current = Cursors.WaitCursor;
			}
			else
			{
				Cursor.Current = Cursors.Default;
			}
		}

		public int ClassGroupSortFunction( ClassGroup Left, ClassGroup Right )
		{
			return ( string.Compare( Left.Name, Right.Name, StringComparison.InvariantCultureIgnoreCase ) );
		}

		private void PopulateClassGroups()
		{
			// Clear out the old list (if any)
			FilterClassesDropDownButton.DropDownItems.Clear();

			// Add in each classgroup alphabetically
			Options.ClassGroups.Sort( new Comparison<ClassGroup>( ClassGroupSortFunction ) );
			foreach( ClassGroup FilterClass in Options.ClassGroups )
			{
				ToolStripMenuItem MenuItem = new ToolStripMenuItem( FilterClass.Name );
				MenuItem.CheckOnClick = true;
				MenuItem.Checked = true;
				MenuItem.Tag = FilterClass;
				MenuItem.CheckState = FilterClass.bFilter ? CheckState.Checked : CheckState.Unchecked;
				MenuItem.Click += new System.EventHandler( ClassesToFilterClick );

				FilterClassesDropDownButton.DropDownItems.Add( MenuItem );
			}

			// Add in the all and none options
			ToolStripSeparator Separator = new ToolStripSeparator();
			FilterClassesDropDownButton.DropDownItems.Add( Separator );

			ToolStripMenuItem AllMenuItem = new ToolStripMenuItem( "All" );
			AllMenuItem.Click += new System.EventHandler( FilterAllClick );
			FilterClassesDropDownButton.DropDownItems.Add( AllMenuItem );

			ToolStripMenuItem NoneMenuItem = new ToolStripMenuItem( "None" );
			NoneMenuItem.Click += new System.EventHandler( FilterNoneClick );
			FilterClassesDropDownButton.DropDownItems.Add( NoneMenuItem );
		}

        /**
         * Assembly resolve method to pick correct StandaloneSymbolParser DLL
         */
		private Assembly CurrentDomain_AssemblyResolve(Object sender, ResolveEventArgs args)
		{
			// Name is fully qualified assembly definition - e.g. "p4dn, Version=1.0.0.0, Culture=neutral, PublicKeyToken=ff968dc1933aba6f"
			string[] AssemblyInfo = args.Name.Split( ",".ToCharArray() );
			string AssemblyName = AssemblyInfo[0];

			if (AssemblyName.ToLower() == "standalonesymbolparser")
			{
                if (IntPtr.Size == 8)
                {
                    AssemblyName = Application.StartupPath + "\\Win64\\" + AssemblyName + ".dll";
                }
                else
                {
                    AssemblyName = Application.StartupPath + "\\Win32\\" + AssemblyName + ".dll";
                }

				Debug.WriteLineIf( System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName );

				Assembly SymParser = Assembly.LoadFile( AssemblyName );
                return SymParser;
			}

			return ( null );
		}


		/**
		 * Updates current snapshot based on selected values in diff start/ end combo boxes.
		 */
		private void UpdateCurrentSnapshot()
		{
            if (SnapshotList == null)
            {
                return;
            }

			// Start == "Start"
			if( DiffStartComboBox.SelectedIndex == 0 )
			{
				// End == Start == "Start"
				if( DiffEndComboBox.SelectedIndex == 0 )
				{
					// "Start" is a pseudo snapshot so there is nothing to compare here.
					CurrentSnapshot = null;
				}
				else
				{
					// Comparing to "Start" means we can simply use the current index (-1 as Start doesn't have index associated).
					CurrentSnapshot = SnapshotList[DiffEndComboBox.SelectedIndex-1];
				}
			}
			// Start != "Start"
			else
			{
				// End == "Start"
				if( DiffEndComboBox.SelectedIndex == 0 )
				{
					// Comparing to "Start" means we can simply use the current index (-1 as Start doesn't have index associated).
					CurrentSnapshot = SnapshotList[DiffStartComboBox.SelectedIndex-1];
				}
				// Start != End and both are != "Start"
				else
				{
					// Do a real diff in this case and create a new snapshot.
					CurrentSnapshot = FStreamSnapshot.DiffSnapshots(SnapshotList[DiffStartComboBox.SelectedIndex-1],SnapshotList[DiffEndComboBox.SelectedIndex-1]);
				}
			}
		}

		/**
		 * Parses the data stream into the current view.
		 */
		private void ParseCurrentView()
		{
			// Update the current snapshot based on combo box settings.
			UpdateCurrentSnapshot();

			// If valid, parse it into the call graph view tree.
			if( CurrentSnapshot != null )
			{
				bool bShouldSortBySize = ( SortCriteriaSplitButton.Text == "Sort by Size" );
                
				List<FCallStackAllocationInfo> CallStackList = null;
				if( AllocTypeSplitButton.Text == "Active Allocations" )
				{
					CallStackList = CurrentSnapshot.ActiveCallStackList;
				}
				else
				{
					CallStackList = CurrentSnapshot.LifetimeCallStackList;
				}

				// Parse snapshots into views.
				// updating the call graph view is WAY SLOW
				if( CallGraphTreeView.Visible )
				{
					FCallGraphTreeViewParser.ParseSnapshot(CallGraphTreeView, CallStackList, this, bShouldSortBySize, ParentFunctionIndex, FilterTextBox.Text.ToUpperInvariant());
				}

				if (ExclusiveListView.Visible)
				{
					FExclusiveListViewParser.ParseSnapshot(ExclusiveListView, CallStackList, this, bShouldSortBySize, FilterTextBox.Text.ToUpperInvariant());
				}

				if( TimeLineChart.Visible )
				{
					FTimeLineChartView.ParseSnapshot( TimeLineChart, CurrentSnapshot );
				}
			}
			else
			{
				// Clear the views to signal error
				ResetViews();
			}

			UpdateStatus("Displaying " + CurrentFilename);
		}

		private void GoButton_Click(object sender,EventArgs e)
		{
			SetWaitCursor( true );
			ParseCurrentView();
			SetWaitCursor( false );
		}

		/**
		 * Resets combobox items and various views into data
		 */ 
		private void ResetComboBoxAndViews()
		{
			// Reset combobox items.
			DiffStartComboBox.Items.Clear();
			DiffEndComboBox.Items.Clear();
			// Reset views.
			ResetViews();
		}
		
		/**
		 *  Resets the various views into the data
		 */
		private void ResetViews()
		{
			// Reset callgraph tree.
			CallGraphTreeView.Nodes.Clear();
			// Reset exclusive list view.
			ExclusiveListView.Items.Clear();
			// Reset call graph view in exclusive tab.
			ExclusiveSingleCallStackView.Items.Clear();
		}

		private void openToolStripMenuItem_Click(object sender,EventArgs e)
		{
			// Create a file open dialog for selecting the .mprof file.
			OpenFileDialog OpenMProfFileDialog = new OpenFileDialog();
			OpenMProfFileDialog.Title = "Open the profile data file from the game's 'Profiling' folder";
			OpenMProfFileDialog.Filter = "Profiling Data (*.mprof)|*.mprof";
			OpenMProfFileDialog.RestoreDirectory = false;
			OpenMProfFileDialog.SupportMultiDottedExtensions = true;
			if( OpenMProfFileDialog.ShowDialog() == DialogResult.OK )
			{
				// Reset combobox items and various views into data
				ResetComboBoxAndViews();

				ParseFile( OpenMProfFileDialog.FileName );
			}
		}

		private void ParseFile( string InCurrentFilename )
		{
			CurrentFilename = InCurrentFilename;

			// Only parse if we have a valid file.
			if (CurrentFilename != "")
			{
				try
				{
                    // Create a new stream info from the opened file.
				    FStreamInfo.GlobalInstance = new FStreamInfo(CurrentFilename);

                    // Parse the token stream and metadata.
                    ProgressDialog.OnBeginBackgroundWork = delegate(BackgroundWorker BGWorker)
                    {
                        SnapshotList = FStreamParser.Parse(this, BGWorker);
                    };
                    if (ProgressDialog.ShowDialog() != DialogResult.OK)
                    {
                        UpdateStatus("Failed to parse '" + CurrentFilename + "', due to '" + ProgressDialog.ExceptionResult + "'");
                        return;
                    }


					// Add Start, End values and user generated snapshots in between.
					DiffStartComboBox.Items.Add("Start");
					DiffEndComboBox.Items.Add("Start");
					// Count-1 as End is implicit last snapshot in list.
					for (int i = 0; i < SnapshotList.Count - 1; i++)
					{
						string SnapshotDescription = "#" + i;
                        if (SnapshotList[i].bHasUserDescription)
                        {
                            SnapshotDescription += ": " + SnapshotList[i].Description;
                        }
						DiffStartComboBox.Items.Add(SnapshotDescription);
						DiffEndComboBox.Items.Add(SnapshotDescription);
					}
					DiffStartComboBox.Items.Add("End");
					DiffEndComboBox.Items.Add("End");

					// Start defaults to "Start" and End defaults to "End" being selected.
					DiffStartComboBox.SelectedIndex = 0;
					DiffEndComboBox.SelectedIndex = DiffEndComboBox.Items.Count - 1;
				}
				catch (Exception ex)
				{
                    UpdateStatus("Failed to parse '" + CurrentFilename + "', due to '" + ex.Message + "'");
					// Reset combobox items and various views into data
					ResetComboBoxAndViews();
				}
			}

			UpdateStatus("Parsed " + CurrentFilename);
		}

		private void exportToCSVToolStripMenuItem_Click(object sender,EventArgs e)
		{
			// Bring up dialog for user to pick filename to export data to.
			SaveFileDialog ExportToCSVFileDialog = new SaveFileDialog();
			ExportToCSVFileDialog.Title = "Export memory profiling data to a .csv file";
			ExportToCSVFileDialog.Filter = "CallGraph in CSV (*.csv)|*.csv";
			ExportToCSVFileDialog.RestoreDirectory = false;
			ExportToCSVFileDialog.SupportMultiDottedExtensions = false;
			ExportToCSVFileDialog.ShowDialog();

			// Determine whether to export lifetime or active allocations.
			bool bShouldExportActiveAllocations = ( AllocTypeSplitButton.Text == "Active Allocations" );
			// Export call graph from current snapshot to CSV.
			CurrentSnapshot.ExportToCSV( ExportToCSVFileDialog.FileName, bShouldExportActiveAllocations );
		}

		/**
		 * Updates the status string label with the passed in string
		 * 
		 * @param	Status		New status to set
		 */
		public void UpdateStatus( string Status )
		{
			StatusStripLabel.Text = Status;
			Refresh();
		}

		/**
		 * Update 2nd list view with full graph of selected row.
		 */ 
		private void ExclusiveListView_SelectedIndexChanged(object sender,EventArgs e)
		{
			ExclusiveSingleCallStackView.BeginUpdate();
			ExclusiveSingleCallStackView.Items.Clear();

			// We can only display a single selected element.
			if( ExclusiveListView.SelectedItems.Count == 1 )
			{
				FCallStackAllocationInfo AllocationInfo = (FCallStackAllocationInfo) ExclusiveListView.SelectedItems[0].Tag;
				FStreamInfo.GlobalInstance.CallStackArray[AllocationInfo.CallStackIndex].AddToListView( ExclusiveSingleCallStackView, true );
			}

			ExclusiveSingleCallStackView.EndUpdate();

            // Sum up the size of all selected items
			int TotalSize = 0;
			int TotalCount = 0;
			foreach( ListViewItem Item in ExclusiveListView.SelectedItems )
			{
				int Size, Count;
				if( int.TryParse( Item.SubItems[0].Text, out Size ) && int.TryParse( Item.SubItems[2].Text, out Count ) )
				{
					TotalSize += Size;
					TotalCount += Count;
				}
			}

            if (ExclusiveListView.SelectedItems.Count > 0)
            {
                string suffix = " KB in selection (in " + TotalCount.ToString() + " allocations)";
				SelectionSizeStatusLabel.Text = TotalSize.ToString( "N0" ) + suffix;
            }
            else
            {
                SelectionSizeStatusLabel.Text = "";
            }
		}

		private void CallGraphTreeView_NodeMouseClick(object sender,TreeNodeMouseClickEventArgs EventArgs)
		{
			if( EventArgs.Button == MouseButtons.Right )
			{
				FNodePayload Payload = (FNodePayload) EventArgs.Node.Tag;
				if( Payload != null )
				{
					ParentFunctionIndex = FStreamInfo.GlobalInstance.CallStackAddressArray[Payload.AddressIndex].FunctionIndex;
					ParentLabel.Text = FStreamInfo.GlobalInstance.NameArray[ParentFunctionIndex];
				}
				// Reparse with updated filter.
				ParseCurrentView();
			}
		}

		private void QuickStartMenuClick( object sender, EventArgs e )
		{
			MessageBox.Show( "Define USE_MALLOC_PROFILER 1 in UMemoryDefines.h\n\nWhile running, type SNAPSHOTMEMORY at suitable intervals. At the end of profiling, type DUMPALLOCSTOFILE to save the .mprof file.\n\nLoad in the .mprof file from <Name>Game/Profiling/<timestamp>/<Name>-<timestamp>.mprof", "Memory Profiler Quick Start", MessageBoxButtons.OK, MessageBoxIcon.Information );
		}

		private void ClearFilterButton_Click(object sender, EventArgs e)
		{
            ParentLabel.Text = "Right click on entry in callstack to populate filter";
			ParentFunctionIndex = -1;
			// Reparse with updated filter.
			ParseCurrentView();
		}

        class ColumnSorter : IComparer
        {
            public int ColumnToSortBy = 0;
            public bool ColumnSortModeAscending = false;

            public IComparer ElementComparer = new CaseInsensitiveComparer();


            public int Compare(Object in_a, Object in_b)
            {
                ListViewItem a = (ListViewItem)in_a;
                ListViewItem b = (ListViewItem)in_b;

                string lhs = a.SubItems[ColumnToSortBy].Text;
                string rhs = b.SubItems[ColumnToSortBy].Text;

                long lhs_int;
                long rhs_int;
                if (long.TryParse(lhs, out lhs_int) && long.TryParse(rhs, out rhs_int))
                {
                    return ElementComparer.Compare(lhs_int, rhs_int) * (ColumnSortModeAscending ? 1 : -1);
                }
                else
                {
                    return ElementComparer.Compare(lhs, rhs) * (ColumnSortModeAscending ? 1 : -1);
                }
            }

            public void OnColumnClick(ListView sender, ColumnClickEventArgs e)
            {
                if (e.Column == ColumnToSortBy)
                {
                    ColumnSortModeAscending = !ColumnSortModeAscending;
                }
                else
                {
                    ColumnToSortBy = e.Column;
                    ColumnSortModeAscending = true;
                }

                sender.Sort();
            }
        }

        private void ExclusiveListView_ColumnClick(object sender, ColumnClickEventArgs e)
        {
            ListView TrueSender = sender as ListView;
            if (TrueSender != null)
            {
                ColumnSorter sorter = TrueSender.ListViewItemSorter as ColumnSorter;
                if (sorter != null)
                {
                    sorter.OnColumnClick(TrueSender, e);
                }
            }
        }

        protected EnvDTE.DTE FindOrCreateDTEInstance(string objectName)
        {
            // Try to find an open copy, and if that fails, create one
	        EnvDTE.DTE result;
	        try
	        {
		        result = System.Runtime.InteropServices.Marshal.GetActiveObject(objectName) as EnvDTE.DTE;
	        }
	        catch
	        {
		        System.Type t = System.Type.GetTypeFromProgID(objectName);
                result = System.Activator.CreateInstance(t) as EnvDTE.DTE;
            }
        	
	        return result;
        }

        protected void OpenFileInVisualStudio(string filename, int lineNumber)
        {
	        // Get a copy of VS running
            EnvDTE.DTE devenv = FindOrCreateDTEInstance("VisualStudio.DTE");
	        if (devenv == null)
	        {
		        // Couldn't get VS running at all!
		        return;
	        }
        	
	        // Make sure the window is visible and prevent it from disappearing
	        devenv.MainWindow.Visible = true;
	        devenv.UserControl = true;
        	
	        // Open the file
            Window win = devenv.ItemOperations.OpenFile(filename, EnvDTE.Constants.vsViewKindTextView);
        	
	        // Go to the desired line number
	        if (lineNumber >= 0)
	        {
		        TextSelection selection = devenv.ActiveDocument.Selection as TextSelection;
		        selection.GotoLine(lineNumber, true);
	        }
        }

		private void SplitButtonClick( object sender, EventArgs e )
		{
			ToolStripSplitButton Button = ( ToolStripSplitButton )sender;
			if( Button.DropDownItems.Count > 1 )
			{
				// Update the button to the next entry in the split button drop down item collection
				bool bFoundEntry = false;
				foreach( ToolStripMenuItem MenuItem in Button.DropDownItems )
				{
					if( bFoundEntry )
					{
						Button.Text = MenuItem.Text;
						bFoundEntry = false;
					}
					else if( Button.Text == MenuItem.Text )
					{
						bFoundEntry = true;
					}
				}

				// Handle case of looping around to the beginning
				if( bFoundEntry )
				{
					Button.Text = Button.DropDownItems[0].Text;
				}
			}
		}

		private void SortByClick( object sender, EventArgs e )
		{
			ToolStripMenuItem Item = ( ToolStripMenuItem )sender;
			SortCriteriaSplitButton.Text = Item.Text;
		}

		private void AllocTypeClick( object sender, EventArgs e )
		{
			ToolStripMenuItem Item = ( ToolStripMenuItem )sender;
			AllocTypeSplitButton.Text = Item.Text;
		}

		private void TemplatesClick( object sender, EventArgs e )
		{
			ToolStripMenuItem Item = ( ToolStripMenuItem )sender;
			ContainersSplitButton.Text = Item.Text;
		}

		private void FilterTypeClick( object sender, EventArgs e )
		{
			ToolStripMenuItem Item = ( ToolStripMenuItem )sender;
			FilterTypeSplitButton.Text = Item.Text;
		}

		private void SetAllClasses( bool State )
		{
			foreach( ToolStripItem Item in FilterClassesDropDownButton.DropDownItems )
			{
				ToolStripMenuItem MenuItem = Item as ToolStripMenuItem;
				if( MenuItem != null )
				{
					ClassGroup FilterClass = ( ClassGroup )Item.Tag;
					if( FilterClass != null )
					{
						MenuItem.Checked = State;
						FilterClass.bFilter = State;
					}
				}
			}
		}

		private void FilterAllClick( object sender, EventArgs e )
		{
			SetAllClasses( true );
			FilterClassesDropDownButton.ShowDropDown();
		}

		private void FilterNoneClick( object sender, EventArgs e )
		{
			SetAllClasses( false );
			FilterClassesDropDownButton.ShowDropDown();
		}

		private void ClassesToFilterClick( object sender, EventArgs e )
		{
			ToolStripMenuItem Item = ( ToolStripMenuItem )sender;
			ClassGroup FilterClass = ( ClassGroup )Item.Tag;
			if( FilterClass != null )
			{
				FilterClass.bFilter = Item.Checked;
			}

			// Make the drop down list not go away when clicked (DismissWhenClicked is unsettable)
			FilterClassesDropDownButton.ShowDropDown();
		}

        // 0 == deepest part of callstack
        void OpenCallstackEntryInVS(FCallStackAllocationInfo AllocationInfo, int index)
        {
            FCallStack CallStack = FStreamInfo.GlobalInstance.CallStackArray[AllocationInfo.CallStackIndex];
            FCallStackAddress Address = FStreamInfo.GlobalInstance.CallStackAddressArray[CallStack.AddressIndices[CallStack.AddressIndices.Count - 1 - index]];
            string filename = FStreamInfo.GlobalInstance.NameArray[Address.FilenameIndex];
            int lineNumber = Address.LineNumber;

            try
            {
                OpenFileInVisualStudio(filename, lineNumber);
            }
            catch (Exception)
            {
            }
        }

        private void ExclusiveListView_DoubleClick(object sender, EventArgs e)
        {
            if (ExclusiveListView.SelectedItems.Count > 0)
            {
                FCallStackAllocationInfo AllocationInfo = (FCallStackAllocationInfo)ExclusiveListView.SelectedItems[0].Tag;
                OpenCallstackEntryInVS(AllocationInfo, 0);
            }
        }

		private void OptionsToolStripMenuItem_Click( object sender, EventArgs e )
		{
			OptionsDialog DisplayOptions = new OptionsDialog( this, Options );
			DisplayOptions.ShowDialog();

			PopulateClassGroups();
		}

		private void MainWindowFormClosing( object sender, FormClosingEventArgs e )
		{
			WriteOptions<SettableOptions>( Options, Path.Combine( Application.StartupPath, "MemoryProfiler2.ClassGroups.xml" ) );
		}

		protected void XmlSerializer_UnknownAttribute( object sender, XmlAttributeEventArgs e )
		{
		}

		protected void XmlSerializer_UnknownNode( object sender, XmlNodeEventArgs e )
		{
		}

		private T ReadOptions<T>( string OptionsFileName ) where T : new()
		{
			T Instance = new T();
			Stream XmlStream = null;
			try
			{
				// Get the XML data stream to read from
				XmlStream = new FileStream( OptionsFileName, FileMode.Open, FileAccess.Read, FileShare.None, 256 * 1024, false );

				// Creates an instance of the XmlSerializer class so we can read the settings object
				XmlSerializer ObjSer = new XmlSerializer( typeof( T ) );
				// Add our callbacks for a busted XML file
				ObjSer.UnknownNode += new XmlNodeEventHandler( XmlSerializer_UnknownNode );
				ObjSer.UnknownAttribute += new XmlAttributeEventHandler( XmlSerializer_UnknownAttribute );

				// Create an object graph from the XML data
				Instance = ( T )ObjSer.Deserialize( XmlStream );
			}
			catch( Exception E )
			{
				System.Diagnostics.Debug.WriteLine( E.Message );
			}
			finally
			{
				if( XmlStream != null )
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}

			return ( Instance );
		}

		private void WriteOptions<T>( T Data, string OptionsFileName )
		{
			Stream XmlStream = null;
			try
			{
				XmlStream = new FileStream( OptionsFileName, FileMode.Create, FileAccess.Write, FileShare.None, 256 * 1024, false );
				XmlSerializer ObjSer = new XmlSerializer( typeof( T ) );

				// Add our callbacks for a busted XML file
				ObjSer.UnknownNode += new XmlNodeEventHandler( XmlSerializer_UnknownNode );
				ObjSer.UnknownAttribute += new XmlAttributeEventHandler( XmlSerializer_UnknownAttribute );

				ObjSer.Serialize( XmlStream, Data );
			}
			catch( Exception E )
			{
				System.Diagnostics.Debug.WriteLine( E.Message );
			}
			finally
			{
				if( XmlStream != null )
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}
		}
	}

	public class ClassGroup
	{
		[DescriptionAttribute( "The name of this class collection." )]
		[XmlAttribute]
		public string Name { get; set; }

		[DescriptionAttribute( "Whether to include this class group in the query." )]
		[XmlAttribute]
		public bool bFilter { get; set; }

		[DescriptionAttribute( "A list of classes to ignore in the memory profile." )]
		[XmlAttribute]
		public string[] ClassNames { get; set; }

		public ClassGroup()
		{
			Name = "<Unknown>";
			bFilter = false;
			ClassNames = null;
		}
	}

	public class SettableOptions
	{
		[CategoryAttribute( "Settings" )]
		[DescriptionAttribute( "A list of class groups to define the callstack filtering." )]
		public List<ClassGroup> ClassGroups { get; set; }
	}
}