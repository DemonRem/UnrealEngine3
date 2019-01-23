using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using Stats;

namespace StatsViewer
{
	/// <summary>
	/// This view provides a call graph view of the stats data on a per frame
	/// basis. It's used to drill down through a set of stats in order to find
	/// spikes in a given stat's time.
	/// </summary>
	public class PerFrameView : System.Windows.Forms.Form
	{
		/// <summary>
		/// The frame window that created this view. This window must be notified
		/// when this form is closed.
		/// </summary>
		private StatsViewerFrame StatsViewer;
		#region Windows Form Designer generated code
		private System.Windows.Forms.TreeView CallGraphTree;
        private ToolStrip toolStrip1;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		#endregion

		private ArrayList Frames;
		private CheckBox ShowHierarchy;
		private CheckBox ShowAllStats;
        private string SelectedStat = null;

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PerFrameView));
			this.CallGraphTree = new System.Windows.Forms.TreeView();
			this.toolStrip1 = new System.Windows.Forms.ToolStrip();
			this.ShowHierarchy = new System.Windows.Forms.CheckBox();
			this.ShowAllStats = new System.Windows.Forms.CheckBox();
			this.SuspendLayout();
			// 
			// CallGraphTree
			// 
			this.CallGraphTree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.CallGraphTree.Location = new System.Drawing.Point(0, 25);
			this.CallGraphTree.Name = "CallGraphTree";
			this.CallGraphTree.Size = new System.Drawing.Size(709, 514);
			this.CallGraphTree.TabIndex = 0;
			// 
			// toolStrip1
			// 
			this.toolStrip1.Location = new System.Drawing.Point(0, 0);
			this.toolStrip1.Name = "toolStrip1";
			this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
			this.toolStrip1.Size = new System.Drawing.Size(709, 25);
			this.toolStrip1.TabIndex = 1;
			this.toolStrip1.Text = "toolStrip1";
			// 
			// ShowHierarchy
			// 
			this.ShowHierarchy.AutoSize = true;
			this.ShowHierarchy.Checked = true;
			this.ShowHierarchy.CheckState = System.Windows.Forms.CheckState.Checked;
			this.ShowHierarchy.Location = new System.Drawing.Point(12, 4);
			this.ShowHierarchy.Name = "ShowHierarchy";
			this.ShowHierarchy.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.ShowHierarchy.Size = new System.Drawing.Size(101, 17);
			this.ShowHierarchy.TabIndex = 2;
			this.ShowHierarchy.Text = "Show Hierarchy";
			this.ShowHierarchy.UseVisualStyleBackColor = true;
			this.ShowHierarchy.CheckedChanged += new System.EventHandler(this.OnShowHierarchy);
			// 
			// ShowAllStats
			// 
			this.ShowAllStats.AutoSize = true;
			this.ShowAllStats.Location = new System.Drawing.Point(139, 4);
			this.ShowAllStats.Name = "ShowAllStats";
			this.ShowAllStats.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
			this.ShowAllStats.Size = new System.Drawing.Size(94, 17);
			this.ShowAllStats.TabIndex = 3;
			this.ShowAllStats.Text = "Show All Stats";
			this.ShowAllStats.UseVisualStyleBackColor = true;
			this.ShowAllStats.CheckedChanged += new System.EventHandler(this.OnShowAllStats);
			// 
			// PerFrameView
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(709, 539);
			this.Controls.Add(this.ShowAllStats);
			this.Controls.Add(this.ShowHierarchy);
			this.Controls.Add(this.toolStrip1);
			this.Controls.Add(this.CallGraphTree);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "PerFrameView";
			this.Text = "Call Graph View";
			this.Closing += new System.ComponentModel.CancelEventHandler(this.PerFrameView_Closing);
			this.ResumeLayout(false);
			this.PerformLayout();

		}
		#endregion

		/// <summary>
		/// Constructs the new view for the specified range of frames
		/// </summary>
		/// <param name="Owner">The window that owns this view</param>
		/// <param name="Frames">The list of frames to add to the view</param>
		public PerFrameView(StatsViewerFrame Owner,ArrayList InFrames)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();
			// Used to notify the owner when we close
			StatsViewer = Owner;
			// Build the tree(s) from the data
            Frames = InFrames;
			BuildTreeData(Frames);
		}

        private TreeNode FindNode( TreeNodeCollection Nodes, string Name )
        {
            foreach (TreeNode ExistingNode in Nodes)
            {
                if (ExistingNode.Text == Name)
                {
                    return ExistingNode;
                }
            }
            return null;
        }

        private TreeNode FindNodeByTag(TreeNodeCollection Nodes, string Tag)
        {
            foreach (TreeNode ExistingNode in Nodes)
            {
                if ( (string)ExistingNode.Tag == Tag)
                {
                    return ExistingNode;
                }
            }
            return null;
        }

		/// <summary>
		/// Builds the call graph tree using the per frame data
		/// </summary>
		/// <param name="Frames">The list of frames to add to the tree</param>
		private void BuildTreeData(ArrayList Frames)
		{
			// Start UI changes
			Cursor = Cursors.WaitCursor;
			CallGraphTree.BeginUpdate();
			try
			{
				CallGraphTree.Nodes.Clear();
				// Add the node the frame data is added to
				TreeNode Root = new TreeNode("Frames");
				CallGraphTree.Nodes.Add(Root);
				// For each frame in the list, recursively add their data
				foreach (Frame frame in Frames)
				{
                    // Add the frame number here
                    TreeNode FrameNode = new TreeNode("Frame " + frame.FrameNumber.ToString());
                    Root.Nodes.Add(FrameNode);

                    // Look for all root stats in this frame
                    foreach (Stat stat in frame.Stats)
                    {
                        // if it has no parent, it's a thread root
                        if (stat.ParentStat == null && stat.Type == Stat.StatType.STATTYPE_CycleCounter)
                        {
                            // make a node name for the thread
                            string ThreadName = "Thread " + stat.ThreadId.ToString();

                            // look for a node already made for this Thread.
                            TreeNode ThreadNode = FindNode(FrameNode.Nodes, ThreadName);

                            // if we didn't find one, make a new one for the thread
                            if (ThreadNode == null)
                            {
                                // Add the thread node here
                                ThreadNode = new TreeNode(ThreadName);
                                FrameNode.Nodes.Add(ThreadNode);
                            }

                            // add this thread 'root' stat to the thread node
                            AddStatToNode(stat, ThreadNode);
                        }
                    }

                    // Make sure the timing info is visible
					FrameNode.Expand();
				}
				// Make sure the newly added frames are visible
				Root.Expand();
			}
			catch (Exception e)
			{
				Console.WriteLine("Exception: \r\n" + e.ToString());
			}
			finally
			{
				// Finalize UI changes
				CallGraphTree.EndUpdate();
				Cursor = Cursors.Default;
			}
		}

		/// <summary>
		/// Adds a stat to the tree node recursively
		/// </summary>
		/// <param name="stat"></param>
		/// <param name="Parent"></param>
		private void AddStatToNode(Stat stat,TreeNode Parent)
		{
			if (ShowHierarchy.Checked)
			{
				if (ShowAllStats.Checked || stat.ValueInMS > 0.0009)
				{
					TreeNode StatNode = new TreeNode(stat.Name + " " +
					stat.ValueInMS.ToString("F3") + " ms   (" +
					stat.CallsPerFrame.ToString() + ")");

					// Add the stat to its parent
					Parent.Nodes.Add(StatNode);

					// Maintain selection
					if (SelectedStat != null && StatNode.Text == SelectedStat)
					{
						CallGraphTree.SelectedNode = StatNode;
					}

					// Now add each child to the new stat node (parent)
					foreach (Stat Child in stat.Children)
					{
						AddStatToNode(Child, StatNode);
					}
				}
			}
			else
			{
				TreeNode GroupNode = FindNode( Parent.Nodes, stat.OwningGroup.Name );

				// Did we encounter a new group?
				if ( GroupNode == null )
				{
					// Reset the values
					foreach (Stat Child in stat.OwningGroup.OwnedStats)
					{
						Child.ValueInMS = 0.0;
						Child.CallsPerFrame = 0;
					}
				}

				// Update the stat value.
				double ValueInMs = 0.0;
				int CallsPerFrame = 0;
				foreach (Stat Child in stat.OwningGroup.OwnedStats)
				{
					if (Child.Name == stat.Name)
					{
						Child.ValueInMS += stat.ValueInMS;
						Child.CallsPerFrame += stat.CallsPerFrame;
						ValueInMs = Child.ValueInMS;
						CallsPerFrame = Child.CallsPerFrame;
						break;
					}
				}

				if (ShowAllStats.Checked || stat.ValueInMS > 0.0009)
				{
					// Add the group if necessary?
					if (GroupNode == null)
					{
						GroupNode = new TreeNode(stat.OwningGroup.Name);
						Parent.Nodes.Add(GroupNode);
					}

					// Find the stat in the group.
					TreeNode StatNode = FindNodeByTag(GroupNode.Nodes, stat.Name);

					// Did we encounter a new stat?
					if (StatNode == null)
					{
						// Add the stat
						StatNode = new TreeNode(stat.Name);
						StatNode.Tag = stat.Name;
						GroupNode.Nodes.Add(StatNode);
					}

					// Update the text.
					StatNode.Text = stat.Name + " " +
									ValueInMs.ToString("F3") + " ms   (" +
									CallsPerFrame.ToString() + ")";

					// Maintain selection
					if (SelectedStat != null && StatNode.Text == SelectedStat)
					{
						CallGraphTree.SelectedNode = StatNode;
					}
				}

				// Now add each child to the new stat node (parent)
				foreach (Stat Child in stat.Children)
				{
					AddStatToNode(Child, Parent);
				}
			}
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}
		
		/// <summary>
		/// Need to notify the main window that we are closing so it can remove the
		/// this view from the list it maintains
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void PerFrameView_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			StatsViewer.RemovePerFrameView(this);
		}

        private void OnShowAllStats(object sender, EventArgs e)
        {
            TreeNode SelectedNode = CallGraphTree.SelectedNode;
            if ( SelectedNode != null )
            {
                SelectedStat = SelectedNode.Text;
            }
            else
            {
                SelectedStat = null;
            }
            BuildTreeData(Frames);
        }

        private void OnShowHierarchy(object sender, EventArgs e)
        {
            TreeNode SelectedNode = CallGraphTree.SelectedNode;
            if (SelectedNode != null)
            {
                SelectedStat = SelectedNode.Text;
            }
            else
            {
                SelectedStat = null;
            }
            BuildTreeData(Frames);
        }
	}
}
