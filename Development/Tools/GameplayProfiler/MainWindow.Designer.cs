namespace GameplayProfiler
{
    partial class MainWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea2 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series3 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.Series series4 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.PerActorTreeView = new System.Windows.Forms.TreeView();
            this.PerClassTreeView = new System.Windows.Forms.TreeView();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.FrameFunctionListView = new System.Windows.Forms.ListView();
            this.ColumnName = new System.Windows.Forms.ColumnHeader();
            this.ColumnIncl = new System.Windows.Forms.ColumnHeader();
            this.ColumnExcl = new System.Windows.Forms.ColumnHeader();
            this.ColumnCalls = new System.Windows.Forms.ColumnHeader();
            this.ColumnInclPerCall = new System.Windows.Forms.ColumnHeader();
            this.ColumnExclPerCall = new System.Windows.Forms.ColumnHeader();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.ClassHierarchyTreeView = new System.Windows.Forms.TreeView();
            this.tabPage5 = new System.Windows.Forms.TabPage();
            this.FrameFunctionCallGraph = new System.Windows.Forms.TreeView();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.AggregateFunctionListView = new System.Windows.Forms.ListView();
            this.FunctionName = new System.Windows.Forms.ColumnHeader();
            this.Incl = new System.Windows.Forms.ColumnHeader();
            this.MaxInclTime = new System.Windows.Forms.ColumnHeader();
            this.Excl = new System.Windows.Forms.ColumnHeader();
            this.MaxExclTime = new System.Windows.Forms.ColumnHeader();
            this.Calls = new System.Windows.Forms.ColumnHeader();
            this.AvgCallsFrame = new System.Windows.Forms.ColumnHeader();
            this.InclPerFrame = new System.Windows.Forms.ColumnHeader();
            this.ExclPerFrame = new System.Windows.Forms.ColumnHeader();
            this.tabPage6 = new System.Windows.Forms.TabPage();
            this.AggregateFunctionCallGraph = new System.Windows.Forms.TreeView();
            this.button1 = new System.Windows.Forms.Button();
            this.FPSChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.TimeThresholdBox = new System.Windows.Forms.NumericUpDown();
            this.label1 = new System.Windows.Forms.Label();
            this.OpenFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.FrameNumTextBox = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.tabPage5.SuspendLayout();
            this.tabPage4.SuspendLayout();
            this.tabPage6.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.FPSChart)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.TimeThresholdBox)).BeginInit();
            this.SuspendLayout();
            // 
            // tabControl1
            // 
            this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Controls.Add(this.tabPage3);
            this.tabControl1.Controls.Add(this.tabPage5);
            this.tabControl1.Controls.Add(this.tabPage4);
            this.tabControl1.Controls.Add(this.tabPage6);
            this.tabControl1.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tabControl1.Location = new System.Drawing.Point(-2, 273);
            this.tabControl1.Margin = new System.Windows.Forms.Padding(0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(1342, 807);
            this.tabControl1.TabIndex = 0;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.splitContainer1);
            this.tabPage1.Location = new System.Drawing.Point(4, 25);
            this.tabPage1.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.tabPage1.Size = new System.Drawing.Size(1334, 778);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "frame actor/ class call graph";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(3, 4);
            this.splitContainer1.Margin = new System.Windows.Forms.Padding(0);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.PerActorTreeView);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.PerClassTreeView);
            this.splitContainer1.Size = new System.Drawing.Size(1328, 770);
            this.splitContainer1.SplitterDistance = 654;
            this.splitContainer1.SplitterWidth = 5;
            this.splitContainer1.TabIndex = 2;
            // 
            // PerActorTreeView
            // 
            this.PerActorTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.PerActorTreeView.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PerActorTreeView.Location = new System.Drawing.Point(0, 0);
            this.PerActorTreeView.Margin = new System.Windows.Forms.Padding(0);
            this.PerActorTreeView.Name = "PerActorTreeView";
            this.PerActorTreeView.Size = new System.Drawing.Size(654, 770);
            this.PerActorTreeView.TabIndex = 0;
            // 
            // PerClassTreeView
            // 
            this.PerClassTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.PerClassTreeView.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PerClassTreeView.Location = new System.Drawing.Point(0, 0);
            this.PerClassTreeView.Margin = new System.Windows.Forms.Padding(0);
            this.PerClassTreeView.Name = "PerClassTreeView";
            this.PerClassTreeView.Size = new System.Drawing.Size(669, 770);
            this.PerClassTreeView.TabIndex = 1;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.FrameFunctionListView);
            this.tabPage2.Location = new System.Drawing.Point(4, 25);
            this.tabPage2.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Size = new System.Drawing.Size(1334, 778);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "frame function summary";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // FrameFunctionListView
            // 
            this.FrameFunctionListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.ColumnName,
            this.ColumnIncl,
            this.ColumnExcl,
            this.ColumnCalls,
            this.ColumnInclPerCall,
            this.ColumnExclPerCall});
            this.FrameFunctionListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.FrameFunctionListView.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FrameFunctionListView.FullRowSelect = true;
            this.FrameFunctionListView.GridLines = true;
            this.FrameFunctionListView.Location = new System.Drawing.Point(0, 0);
            this.FrameFunctionListView.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.FrameFunctionListView.Name = "FrameFunctionListView";
            this.FrameFunctionListView.Size = new System.Drawing.Size(1334, 778);
            this.FrameFunctionListView.TabIndex = 1;
            this.FrameFunctionListView.UseCompatibleStateImageBehavior = false;
            this.FrameFunctionListView.View = System.Windows.Forms.View.Details;
            this.FrameFunctionListView.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.FrameFunctionListView_ColumnClick);
            // 
            // ColumnName
            // 
            this.ColumnName.Text = "Function Name";
            this.ColumnName.Width = 500;
            // 
            // ColumnIncl
            // 
            this.ColumnIncl.Text = "incl.";
            this.ColumnIncl.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.ColumnIncl.Width = 85;
            // 
            // ColumnExcl
            // 
            this.ColumnExcl.Text = "excl.";
            this.ColumnExcl.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.ColumnExcl.Width = 85;
            // 
            // ColumnCalls
            // 
            this.ColumnCalls.Text = "calls";
            this.ColumnCalls.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.ColumnCalls.Width = 85;
            // 
            // ColumnInclPerCall
            // 
            this.ColumnInclPerCall.Text = "incl. per call";
            this.ColumnInclPerCall.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.ColumnInclPerCall.Width = 130;
            // 
            // ColumnExclPerCall
            // 
            this.ColumnExclPerCall.Text = "excl. per call";
            this.ColumnExclPerCall.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.ColumnExclPerCall.Width = 130;
            // 
            // tabPage3
            // 
            this.tabPage3.Controls.Add(this.ClassHierarchyTreeView);
            this.tabPage3.Location = new System.Drawing.Point(4, 25);
            this.tabPage3.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Size = new System.Drawing.Size(1334, 778);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "frame class hierarchy";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // ClassHierarchyTreeView
            // 
            this.ClassHierarchyTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ClassHierarchyTreeView.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ClassHierarchyTreeView.Location = new System.Drawing.Point(0, 0);
            this.ClassHierarchyTreeView.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.ClassHierarchyTreeView.Name = "ClassHierarchyTreeView";
            this.ClassHierarchyTreeView.Size = new System.Drawing.Size(1334, 778);
            this.ClassHierarchyTreeView.TabIndex = 0;
            // 
            // tabPage5
            // 
            this.tabPage5.Controls.Add(this.FrameFunctionCallGraph);
            this.tabPage5.Location = new System.Drawing.Point(4, 25);
            this.tabPage5.Name = "tabPage5";
            this.tabPage5.Size = new System.Drawing.Size(1334, 778);
            this.tabPage5.TabIndex = 4;
            this.tabPage5.Text = "frame function call graph";
            this.tabPage5.UseVisualStyleBackColor = true;
            // 
            // FrameFunctionCallGraph
            // 
            this.FrameFunctionCallGraph.Dock = System.Windows.Forms.DockStyle.Fill;
            this.FrameFunctionCallGraph.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FrameFunctionCallGraph.Location = new System.Drawing.Point(0, 0);
            this.FrameFunctionCallGraph.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.FrameFunctionCallGraph.Name = "FrameFunctionCallGraph";
            this.FrameFunctionCallGraph.Size = new System.Drawing.Size(1334, 778);
            this.FrameFunctionCallGraph.TabIndex = 1;
            // 
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.AggregateFunctionListView);
            this.tabPage4.Location = new System.Drawing.Point(4, 25);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Size = new System.Drawing.Size(1334, 778);
            this.tabPage4.TabIndex = 3;
            this.tabPage4.Text = "aggr. function summary";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // AggregateFunctionListView
            // 
            this.AggregateFunctionListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.FunctionName,
            this.Incl,
            this.MaxInclTime,
            this.Excl,
            this.MaxExclTime,
            this.Calls,
            this.AvgCallsFrame,
            this.InclPerFrame,
            this.ExclPerFrame});
            this.AggregateFunctionListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.AggregateFunctionListView.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.AggregateFunctionListView.FullRowSelect = true;
            this.AggregateFunctionListView.GridLines = true;
            this.AggregateFunctionListView.Location = new System.Drawing.Point(0, 0);
            this.AggregateFunctionListView.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.AggregateFunctionListView.Name = "AggregateFunctionListView";
            this.AggregateFunctionListView.Size = new System.Drawing.Size(1334, 778);
            this.AggregateFunctionListView.TabIndex = 2;
            this.AggregateFunctionListView.UseCompatibleStateImageBehavior = false;
            this.AggregateFunctionListView.View = System.Windows.Forms.View.Details;
            this.AggregateFunctionListView.SelectedIndexChanged += new System.EventHandler(this.AggregateFunctionListView_SelectedIndexChanged);
            this.AggregateFunctionListView.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.AggregateFunctionListView_ColumnClick);
            this.AggregateFunctionListView.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.AggregateFunctionListView_ItemSelectionChanged_1);
            // 
            // FunctionName
            // 
            this.FunctionName.Text = "Function Name";
            this.FunctionName.Width = 450;
            // 
            // Incl
            // 
            this.Incl.Text = "incl.";
            this.Incl.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.Incl.Width = 85;
            // 
            // MaxInclTime
            // 
            this.MaxInclTime.Text = "max incl.";
            this.MaxInclTime.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.MaxInclTime.Width = 90;
            // 
            // Excl
            // 
            this.Excl.Text = "excl.";
            this.Excl.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.Excl.Width = 85;
            // 
            // MaxExclTime
            // 
            this.MaxExclTime.Text = "max excl.";
            this.MaxExclTime.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.MaxExclTime.Width = 90;
            // 
            // Calls
            // 
            this.Calls.Text = "calls";
            this.Calls.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.Calls.Width = 85;
            // 
            // AvgCallsFrame
            // 
            this.AvgCallsFrame.Text = "avg calls / frame";
            this.AvgCallsFrame.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.AvgCallsFrame.Width = 150;
            // 
            // InclPerFrame
            // 
            this.InclPerFrame.Text = "incl. per frame";
            this.InclPerFrame.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.InclPerFrame.Width = 140;
            // 
            // ExclPerFrame
            // 
            this.ExclPerFrame.Text = "excl. per frame";
            this.ExclPerFrame.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.ExclPerFrame.Width = 140;
            // 
            // tabPage6
            // 
            this.tabPage6.Controls.Add(this.AggregateFunctionCallGraph);
            this.tabPage6.Location = new System.Drawing.Point(4, 25);
            this.tabPage6.Name = "tabPage6";
            this.tabPage6.Size = new System.Drawing.Size(1334, 778);
            this.tabPage6.TabIndex = 5;
            this.tabPage6.Text = "aggr. function call graph";
            this.tabPage6.UseVisualStyleBackColor = true;
            // 
            // AggregateFunctionCallGraph
            // 
            this.AggregateFunctionCallGraph.Dock = System.Windows.Forms.DockStyle.Fill;
            this.AggregateFunctionCallGraph.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.AggregateFunctionCallGraph.Location = new System.Drawing.Point(0, 0);
            this.AggregateFunctionCallGraph.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.AggregateFunctionCallGraph.Name = "AggregateFunctionCallGraph";
            this.AggregateFunctionCallGraph.Size = new System.Drawing.Size(1334, 778);
            this.AggregateFunctionCallGraph.TabIndex = 2;
            this.AggregateFunctionCallGraph.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.AggregateFunctionCallGraph_AfterSelect);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(14, 15);
            this.button1.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(206, 27);
            this.button1.TabIndex = 1;
            this.button1.Text = "File Open";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.FileOpen_Click);
            // 
            // FPSChart
            // 
            this.FPSChart.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.FPSChart.BackColor = System.Drawing.Color.Transparent;
            chartArea2.AxisX.MajorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea2.AxisX.Minimum = 0;
            chartArea2.AxisY.MajorGrid.LineColor = System.Drawing.Color.Silver;
            chartArea2.BackColor = System.Drawing.Color.Transparent;
            chartArea2.CursorX.IsUserEnabled = true;
            chartArea2.CursorX.IsUserSelectionEnabled = true;
            chartArea2.CursorY.IsUserSelectionEnabled = true;
            chartArea2.Name = "DefaultChartArea";
            this.FPSChart.ChartAreas.Add(chartArea2);
            this.FPSChart.Location = new System.Drawing.Point(0, 48);
            this.FPSChart.Margin = new System.Windows.Forms.Padding(0);
            this.FPSChart.Name = "FPSChart";
            this.FPSChart.Palette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.Bright;
            series3.ChartArea = "DefaultChartArea";
            series3.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series3.Name = "FrameTime";
            series3.ToolTip = "Total Frame Time (ms)";
            series4.ChartArea = "DefaultChartArea";
            series4.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series4.Name = "TrackedTime";
            series4.ToolTip = "Tracked Frame Time (ms)";
            this.FPSChart.Series.Add(series3);
            this.FPSChart.Series.Add(series4);
            this.FPSChart.Size = new System.Drawing.Size(1342, 209);
            this.FPSChart.TabIndex = 2;
            this.FPSChart.Text = "chart1";
            this.FPSChart.SelectionRangeChanged += new System.EventHandler<System.Windows.Forms.DataVisualization.Charting.CursorEventArgs>(this.FPSChart_SelectionRangeChanged);
            this.FPSChart.GetToolTipText += new System.EventHandler<System.Windows.Forms.DataVisualization.Charting.ToolTipEventArgs>(this.FPSChart_GetToolTipText);
            // 
            // TimeThresholdBox
            // 
            this.TimeThresholdBox.DecimalPlaces = 1;
            this.TimeThresholdBox.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.TimeThresholdBox.Location = new System.Drawing.Point(270, 18);
            this.TimeThresholdBox.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
            this.TimeThresholdBox.Name = "TimeThresholdBox";
            this.TimeThresholdBox.Size = new System.Drawing.Size(74, 23);
            this.TimeThresholdBox.TabIndex = 4;
            this.TimeThresholdBox.Value = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.TimeThresholdBox.ValueChanged += new System.EventHandler(this.TimeThresholdBox_ValueChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(350, 20);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(129, 16);
            this.label1.TabIndex = 5;
            this.label1.Text = "Time Threshold (ms)";
            // 
            // OpenFileDialog
            // 
            this.OpenFileDialog.DefaultExt = "gprof";
            this.OpenFileDialog.Filter = "gprof files|*.gprof";
            // 
            // FrameNumTextBox
            // 
            this.FrameNumTextBox.Location = new System.Drawing.Point(515, 20);
            this.FrameNumTextBox.Name = "FrameNumTextBox";
            this.FrameNumTextBox.Size = new System.Drawing.Size(74, 23);
            this.FrameNumTextBox.TabIndex = 6;
            this.FrameNumTextBox.TextChanged += new System.EventHandler(this.FrameNumTextBox_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(595, 23);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(45, 16);
            this.label2.TabIndex = 7;
            this.label2.Text = "Frame";
            // 
            // MainWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1337, 1076);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.FrameNumTextBox);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.TimeThresholdBox);
            this.Controls.Add(this.FPSChart);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.tabControl1);
            this.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.Name = "MainWindow";
            this.Text = "Gameplay Profiler";
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.tabPage5.ResumeLayout(false);
            this.tabPage4.ResumeLayout(false);
            this.tabPage6.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.FPSChart)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.TimeThresholdBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TreeView PerActorTreeView;
		private System.Windows.Forms.Button button1;
		private System.Windows.Forms.DataVisualization.Charting.Chart FPSChart;
		private System.Windows.Forms.TreeView PerClassTreeView;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.ListView FrameFunctionListView;
		private System.Windows.Forms.ColumnHeader ColumnName;
		private System.Windows.Forms.ColumnHeader ColumnIncl;
		private System.Windows.Forms.ColumnHeader ColumnExcl;
		private System.Windows.Forms.ColumnHeader ColumnCalls;
		private System.Windows.Forms.ColumnHeader ColumnInclPerCall;
		private System.Windows.Forms.ColumnHeader ColumnExclPerCall;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.TreeView ClassHierarchyTreeView;
		private System.Windows.Forms.SplitContainer splitContainer1;
		private System.Windows.Forms.NumericUpDown TimeThresholdBox;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.OpenFileDialog OpenFileDialog;
		private System.Windows.Forms.TabPage tabPage4;
		private System.Windows.Forms.ListView AggregateFunctionListView;
		private System.Windows.Forms.ColumnHeader FunctionName;
		private System.Windows.Forms.ColumnHeader Incl;
		private System.Windows.Forms.ColumnHeader Excl;
		private System.Windows.Forms.ColumnHeader Calls;
		private System.Windows.Forms.ColumnHeader InclPerFrame;
		private System.Windows.Forms.ColumnHeader ExclPerFrame;
		private System.Windows.Forms.ColumnHeader AvgCallsFrame;
		private System.Windows.Forms.ColumnHeader MaxInclTime;
		private System.Windows.Forms.ColumnHeader MaxExclTime;
		private System.Windows.Forms.TabPage tabPage5;
		private System.Windows.Forms.TreeView FrameFunctionCallGraph;
		private System.Windows.Forms.TabPage tabPage6;
		private System.Windows.Forms.TreeView AggregateFunctionCallGraph;
        private System.Windows.Forms.TextBox FrameNumTextBox;
        private System.Windows.Forms.Label label2;
    }
}

