/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
namespace MemoryProfiler2
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
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager( typeof( MainWindow ) );
			System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
			System.Windows.Forms.DataVisualization.Charting.Legend legend1 = new System.Windows.Forms.DataVisualization.Charting.Legend();
			System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series2 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series3 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series4 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series5 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series6 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series7 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series8 = new System.Windows.Forms.DataVisualization.Charting.Series();
			System.Windows.Forms.DataVisualization.Charting.Series series9 = new System.Windows.Forms.DataVisualization.Charting.Series();
			this.CallGraphTreeView = new System.Windows.Forms.TreeView();
			this.MainMenu = new System.Windows.Forms.MenuStrip();
			this.FileMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.OpenMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ExportToCSVMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ToolMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.OptionsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.HelpMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.QuickStartMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ToolStrip = new System.Windows.Forms.ToolStrip();
			this.DiffStartToolLabel = new System.Windows.Forms.ToolStripLabel();
			this.DiffStartComboBox = new System.Windows.Forms.ToolStripComboBox();
			this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
			this.DiffEndToolLabel = new System.Windows.Forms.ToolStripLabel();
			this.DiffEndComboBox = new System.Windows.Forms.ToolStripComboBox();
			this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
			this.SortCriteriaSplitButton = new System.Windows.Forms.ToolStripSplitButton();
			this.SortBySizeMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.SortByCountMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
			this.AllocTypeSplitButton = new System.Windows.Forms.ToolStripSplitButton();
			this.ActiveAllocsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.LifetimeAllocsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
			this.ContainersSplitButton = new System.Windows.Forms.ToolStripSplitButton();
			this.HideTemplatesMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.ShowTemplatesMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator9 = new System.Windows.Forms.ToolStripSeparator();
			this.FilterTypeSplitButton = new System.Windows.Forms.ToolStripSplitButton();
			this.FilterOutClassesMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.FilterInClassesMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
			this.FilterClassesDropDownButton = new System.Windows.Forms.ToolStripDropDownButton();
			this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
			this.FilterLabel = new System.Windows.Forms.ToolStripLabel();
			this.FilterTextBox = new System.Windows.Forms.ToolStripTextBox();
			this.toolStripSeparator8 = new System.Windows.Forms.ToolStripSeparator();
			this.GoButton = new System.Windows.Forms.ToolStripButton();
			this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
			this.MainStatusStrip = new System.Windows.Forms.StatusStrip();
			this.StatusStripLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.SelectionSizeStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.MainTabControl = new System.Windows.Forms.TabControl();
			this.CallGraphTabPage = new System.Windows.Forms.TabPage();
			this.ParentLabel = new System.Windows.Forms.Label();
			this.ResetParentButton = new System.Windows.Forms.Button();
			this.ExclusiveTabPage = new System.Windows.Forms.TabPage();
			this.ExclusiveSplitContainer = new System.Windows.Forms.SplitContainer();
			this.ExclusiveListView = new System.Windows.Forms.ListView();
			this.SizeInKByte = new System.Windows.Forms.ColumnHeader();
			this.SizePercentage = new System.Windows.Forms.ColumnHeader();
			this.Count = new System.Windows.Forms.ColumnHeader();
			this.CountPercentage = new System.Windows.Forms.ColumnHeader();
			this.GroupName = new System.Windows.Forms.ColumnHeader();
			this.FunctionName = new System.Windows.Forms.ColumnHeader();
			this.ExclusiveSingleCallStackView = new System.Windows.Forms.ListView();
			this.Function = new System.Windows.Forms.ColumnHeader();
			this.File = new System.Windows.Forms.ColumnHeader();
			this.Line = new System.Windows.Forms.ColumnHeader();
			this.TimelineTabPage = new System.Windows.Forms.TabPage();
			this.TimeLineChart = new System.Windows.Forms.DataVisualization.Charting.Chart();
			this.GroupContextMenu = new System.Windows.Forms.ContextMenuStrip( this.components );
			this.GroupMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.UngroupMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.MainMenu.SuspendLayout();
			this.ToolStrip.SuspendLayout();
			this.MainStatusStrip.SuspendLayout();
			this.MainTabControl.SuspendLayout();
			this.CallGraphTabPage.SuspendLayout();
			this.ExclusiveTabPage.SuspendLayout();
			this.ExclusiveSplitContainer.Panel1.SuspendLayout();
			this.ExclusiveSplitContainer.Panel2.SuspendLayout();
			this.ExclusiveSplitContainer.SuspendLayout();
			this.TimelineTabPage.SuspendLayout();
			( ( System.ComponentModel.ISupportInitialize )( this.TimeLineChart ) ).BeginInit();
			this.GroupContextMenu.SuspendLayout();
			this.SuspendLayout();
			// 
			// CallGraphTreeView
			// 
			resources.ApplyResources( this.CallGraphTreeView, "CallGraphTreeView" );
			this.CallGraphTreeView.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.CallGraphTreeView.Name = "CallGraphTreeView";
			this.CallGraphTreeView.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler( this.CallGraphTreeView_NodeMouseClick );
			// 
			// MainMenu
			// 
			this.MainMenu.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.FileMenuItem,
            this.ToolMenuItem,
            this.HelpMenuItem} );
			resources.ApplyResources( this.MainMenu, "MainMenu" );
			this.MainMenu.Name = "MainMenu";
			// 
			// FileMenuItem
			// 
			this.FileMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.OpenMenuItem,
            this.ExportToCSVMenuItem} );
			this.FileMenuItem.Name = "FileMenuItem";
			resources.ApplyResources( this.FileMenuItem, "FileMenuItem" );
			// 
			// OpenMenuItem
			// 
			this.OpenMenuItem.Name = "OpenMenuItem";
			resources.ApplyResources( this.OpenMenuItem, "OpenMenuItem" );
			this.OpenMenuItem.Click += new System.EventHandler( this.openToolStripMenuItem_Click );
			// 
			// ExportToCSVMenuItem
			// 
			this.ExportToCSVMenuItem.Name = "ExportToCSVMenuItem";
			resources.ApplyResources( this.ExportToCSVMenuItem, "ExportToCSVMenuItem" );
			this.ExportToCSVMenuItem.Click += new System.EventHandler( this.exportToCSVToolStripMenuItem_Click );
			// 
			// ToolMenuItem
			// 
			this.ToolMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.OptionsMenuItem} );
			this.ToolMenuItem.Name = "ToolMenuItem";
			resources.ApplyResources( this.ToolMenuItem, "ToolMenuItem" );
			// 
			// OptionsMenuItem
			// 
			this.OptionsMenuItem.Name = "OptionsMenuItem";
			resources.ApplyResources( this.OptionsMenuItem, "OptionsMenuItem" );
			this.OptionsMenuItem.Click += new System.EventHandler( this.OptionsToolStripMenuItem_Click );
			// 
			// HelpMenuItem
			// 
			this.HelpMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.QuickStartMenuItem} );
			this.HelpMenuItem.Name = "HelpMenuItem";
			resources.ApplyResources( this.HelpMenuItem, "HelpMenuItem" );
			// 
			// QuickStartMenuItem
			// 
			this.QuickStartMenuItem.Name = "QuickStartMenuItem";
			resources.ApplyResources( this.QuickStartMenuItem, "QuickStartMenuItem" );
			this.QuickStartMenuItem.Click += new System.EventHandler( this.QuickStartMenuClick );
			// 
			// ToolStrip
			// 
			this.ToolStrip.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.DiffStartToolLabel,
            this.DiffStartComboBox,
            this.toolStripSeparator1,
            this.DiffEndToolLabel,
            this.DiffEndComboBox,
            this.toolStripSeparator2,
            this.SortCriteriaSplitButton,
            this.toolStripSeparator3,
            this.AllocTypeSplitButton,
            this.toolStripSeparator4,
            this.ContainersSplitButton,
            this.toolStripSeparator9,
            this.FilterTypeSplitButton,
            this.toolStripSeparator5,
            this.FilterClassesDropDownButton,
            this.toolStripSeparator7,
            this.FilterLabel,
            this.FilterTextBox,
            this.toolStripSeparator8,
            this.GoButton,
            this.toolStripSeparator6} );
			resources.ApplyResources( this.ToolStrip, "ToolStrip" );
			this.ToolStrip.Name = "ToolStrip";
			// 
			// DiffStartToolLabel
			// 
			this.DiffStartToolLabel.Name = "DiffStartToolLabel";
			resources.ApplyResources( this.DiffStartToolLabel, "DiffStartToolLabel" );
			// 
			// DiffStartComboBox
			// 
			this.DiffStartComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			resources.ApplyResources( this.DiffStartComboBox, "DiffStartComboBox" );
			this.DiffStartComboBox.Name = "DiffStartComboBox";
			// 
			// toolStripSeparator1
			// 
			this.toolStripSeparator1.Name = "toolStripSeparator1";
			resources.ApplyResources( this.toolStripSeparator1, "toolStripSeparator1" );
			// 
			// DiffEndToolLabel
			// 
			this.DiffEndToolLabel.Name = "DiffEndToolLabel";
			resources.ApplyResources( this.DiffEndToolLabel, "DiffEndToolLabel" );
			// 
			// DiffEndComboBox
			// 
			this.DiffEndComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			resources.ApplyResources( this.DiffEndComboBox, "DiffEndComboBox" );
			this.DiffEndComboBox.Name = "DiffEndComboBox";
			// 
			// toolStripSeparator2
			// 
			this.toolStripSeparator2.Name = "toolStripSeparator2";
			resources.ApplyResources( this.toolStripSeparator2, "toolStripSeparator2" );
			// 
			// SortCriteriaSplitButton
			// 
			this.SortCriteriaSplitButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.SortCriteriaSplitButton.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.SortBySizeMenuItem,
            this.SortByCountMenuItem} );
			resources.ApplyResources( this.SortCriteriaSplitButton, "SortCriteriaSplitButton" );
			this.SortCriteriaSplitButton.Name = "SortCriteriaSplitButton";
			this.SortCriteriaSplitButton.ButtonClick += new System.EventHandler( this.SplitButtonClick );
			// 
			// SortBySizeMenuItem
			// 
			this.SortBySizeMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.SortBySizeMenuItem.Name = "SortBySizeMenuItem";
			resources.ApplyResources( this.SortBySizeMenuItem, "SortBySizeMenuItem" );
			this.SortBySizeMenuItem.Click += new System.EventHandler( this.SortByClick );
			// 
			// SortByCountMenuItem
			// 
			this.SortByCountMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.SortByCountMenuItem.Name = "SortByCountMenuItem";
			resources.ApplyResources( this.SortByCountMenuItem, "SortByCountMenuItem" );
			this.SortByCountMenuItem.Click += new System.EventHandler( this.SortByClick );
			// 
			// toolStripSeparator3
			// 
			this.toolStripSeparator3.Name = "toolStripSeparator3";
			resources.ApplyResources( this.toolStripSeparator3, "toolStripSeparator3" );
			// 
			// AllocTypeSplitButton
			// 
			this.AllocTypeSplitButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.AllocTypeSplitButton.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.ActiveAllocsMenuItem,
            this.LifetimeAllocsMenuItem} );
			resources.ApplyResources( this.AllocTypeSplitButton, "AllocTypeSplitButton" );
			this.AllocTypeSplitButton.Name = "AllocTypeSplitButton";
			this.AllocTypeSplitButton.ButtonClick += new System.EventHandler( this.SplitButtonClick );
			// 
			// ActiveAllocsMenuItem
			// 
			this.ActiveAllocsMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.ActiveAllocsMenuItem.Name = "ActiveAllocsMenuItem";
			resources.ApplyResources( this.ActiveAllocsMenuItem, "ActiveAllocsMenuItem" );
			this.ActiveAllocsMenuItem.Click += new System.EventHandler( this.AllocTypeClick );
			// 
			// LifetimeAllocsMenuItem
			// 
			this.LifetimeAllocsMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.LifetimeAllocsMenuItem.Name = "LifetimeAllocsMenuItem";
			resources.ApplyResources( this.LifetimeAllocsMenuItem, "LifetimeAllocsMenuItem" );
			this.LifetimeAllocsMenuItem.Click += new System.EventHandler( this.AllocTypeClick );
			// 
			// toolStripSeparator4
			// 
			this.toolStripSeparator4.Name = "toolStripSeparator4";
			resources.ApplyResources( this.toolStripSeparator4, "toolStripSeparator4" );
			// 
			// ContainersSplitButton
			// 
			this.ContainersSplitButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.ContainersSplitButton.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.HideTemplatesMenuItem,
            this.ShowTemplatesMenuItem} );
			resources.ApplyResources( this.ContainersSplitButton, "ContainersSplitButton" );
			this.ContainersSplitButton.Name = "ContainersSplitButton";
			this.ContainersSplitButton.ButtonClick += new System.EventHandler( this.SplitButtonClick );
			// 
			// HideTemplatesMenuItem
			// 
			this.HideTemplatesMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.HideTemplatesMenuItem.Name = "HideTemplatesMenuItem";
			resources.ApplyResources( this.HideTemplatesMenuItem, "HideTemplatesMenuItem" );
			this.HideTemplatesMenuItem.Click += new System.EventHandler( this.TemplatesClick );
			// 
			// ShowTemplatesMenuItem
			// 
			this.ShowTemplatesMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.ShowTemplatesMenuItem.Name = "ShowTemplatesMenuItem";
			resources.ApplyResources( this.ShowTemplatesMenuItem, "ShowTemplatesMenuItem" );
			this.ShowTemplatesMenuItem.Click += new System.EventHandler( this.TemplatesClick );
			// 
			// toolStripSeparator9
			// 
			this.toolStripSeparator9.Name = "toolStripSeparator9";
			resources.ApplyResources( this.toolStripSeparator9, "toolStripSeparator9" );
			// 
			// FilterTypeSplitButton
			// 
			this.FilterTypeSplitButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.FilterTypeSplitButton.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.FilterOutClassesMenuItem,
            this.FilterInClassesMenuItem} );
			resources.ApplyResources( this.FilterTypeSplitButton, "FilterTypeSplitButton" );
			this.FilterTypeSplitButton.Name = "FilterTypeSplitButton";
			this.FilterTypeSplitButton.ButtonClick += new System.EventHandler( this.SplitButtonClick );
			// 
			// FilterOutClassesMenuItem
			// 
			this.FilterOutClassesMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.FilterOutClassesMenuItem.Name = "FilterOutClassesMenuItem";
			resources.ApplyResources( this.FilterOutClassesMenuItem, "FilterOutClassesMenuItem" );
			this.FilterOutClassesMenuItem.Click += new System.EventHandler( this.FilterTypeClick );
			// 
			// FilterInClassesMenuItem
			// 
			this.FilterInClassesMenuItem.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			this.FilterInClassesMenuItem.Name = "FilterInClassesMenuItem";
			resources.ApplyResources( this.FilterInClassesMenuItem, "FilterInClassesMenuItem" );
			this.FilterInClassesMenuItem.Click += new System.EventHandler( this.FilterTypeClick );
			// 
			// toolStripSeparator5
			// 
			this.toolStripSeparator5.Name = "toolStripSeparator5";
			resources.ApplyResources( this.toolStripSeparator5, "toolStripSeparator5" );
			// 
			// FilterClassesDropDownButton
			// 
			this.FilterClassesDropDownButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			resources.ApplyResources( this.FilterClassesDropDownButton, "FilterClassesDropDownButton" );
			this.FilterClassesDropDownButton.Name = "FilterClassesDropDownButton";
			// 
			// toolStripSeparator7
			// 
			this.toolStripSeparator7.Name = "toolStripSeparator7";
			resources.ApplyResources( this.toolStripSeparator7, "toolStripSeparator7" );
			// 
			// FilterLabel
			// 
			this.FilterLabel.Name = "FilterLabel";
			resources.ApplyResources( this.FilterLabel, "FilterLabel" );
			// 
			// FilterTextBox
			// 
			this.FilterTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.FilterTextBox.Name = "FilterTextBox";
			resources.ApplyResources( this.FilterTextBox, "FilterTextBox" );
			// 
			// toolStripSeparator8
			// 
			this.toolStripSeparator8.Name = "toolStripSeparator8";
			resources.ApplyResources( this.toolStripSeparator8, "toolStripSeparator8" );
			// 
			// GoButton
			// 
			this.GoButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
			resources.ApplyResources( this.GoButton, "GoButton" );
			this.GoButton.Name = "GoButton";
			this.GoButton.Click += new System.EventHandler( this.GoButton_Click );
			// 
			// toolStripSeparator6
			// 
			this.toolStripSeparator6.Name = "toolStripSeparator6";
			resources.ApplyResources( this.toolStripSeparator6, "toolStripSeparator6" );
			// 
			// MainStatusStrip
			// 
			this.MainStatusStrip.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.StatusStripLabel,
            this.SelectionSizeStatusLabel} );
			resources.ApplyResources( this.MainStatusStrip, "MainStatusStrip" );
			this.MainStatusStrip.Name = "MainStatusStrip";
			// 
			// StatusStripLabel
			// 
			this.StatusStripLabel.Name = "StatusStripLabel";
			resources.ApplyResources( this.StatusStripLabel, "StatusStripLabel" );
			// 
			// SelectionSizeStatusLabel
			// 
			this.SelectionSizeStatusLabel.Name = "SelectionSizeStatusLabel";
			resources.ApplyResources( this.SelectionSizeStatusLabel, "SelectionSizeStatusLabel" );
			// 
			// MainTabControl
			// 
			resources.ApplyResources( this.MainTabControl, "MainTabControl" );
			this.MainTabControl.Controls.Add( this.CallGraphTabPage );
			this.MainTabControl.Controls.Add( this.ExclusiveTabPage );
			this.MainTabControl.Controls.Add( this.TimelineTabPage );
			this.MainTabControl.Name = "MainTabControl";
			this.MainTabControl.SelectedIndex = 0;
			// 
			// CallGraphTabPage
			// 
			this.CallGraphTabPage.Controls.Add( this.ParentLabel );
			this.CallGraphTabPage.Controls.Add( this.ResetParentButton );
			this.CallGraphTabPage.Controls.Add( this.CallGraphTreeView );
			resources.ApplyResources( this.CallGraphTabPage, "CallGraphTabPage" );
			this.CallGraphTabPage.Name = "CallGraphTabPage";
			this.CallGraphTabPage.UseVisualStyleBackColor = true;
			// 
			// ParentLabel
			// 
			resources.ApplyResources( this.ParentLabel, "ParentLabel" );
			this.ParentLabel.Name = "ParentLabel";
			// 
			// ResetParentButton
			// 
			resources.ApplyResources( this.ResetParentButton, "ResetParentButton" );
			this.ResetParentButton.Name = "ResetParentButton";
			this.ResetParentButton.UseVisualStyleBackColor = true;
			this.ResetParentButton.Click += new System.EventHandler( this.ClearFilterButton_Click );
			// 
			// ExclusiveTabPage
			// 
			this.ExclusiveTabPage.Controls.Add( this.ExclusiveSplitContainer );
			resources.ApplyResources( this.ExclusiveTabPage, "ExclusiveTabPage" );
			this.ExclusiveTabPage.Name = "ExclusiveTabPage";
			this.ExclusiveTabPage.UseVisualStyleBackColor = true;
			// 
			// ExclusiveSplitContainer
			// 
			resources.ApplyResources( this.ExclusiveSplitContainer, "ExclusiveSplitContainer" );
			this.ExclusiveSplitContainer.Name = "ExclusiveSplitContainer";
			// 
			// ExclusiveSplitContainer.Panel1
			// 
			this.ExclusiveSplitContainer.Panel1.Controls.Add( this.ExclusiveListView );
			// 
			// ExclusiveSplitContainer.Panel2
			// 
			this.ExclusiveSplitContainer.Panel2.Controls.Add( this.ExclusiveSingleCallStackView );
			// 
			// ExclusiveListView
			// 
			this.ExclusiveListView.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.ExclusiveListView.Columns.AddRange( new System.Windows.Forms.ColumnHeader[] {
            this.SizeInKByte,
            this.SizePercentage,
            this.Count,
            this.CountPercentage,
            this.GroupName,
            this.FunctionName} );
			resources.ApplyResources( this.ExclusiveListView, "ExclusiveListView" );
			this.ExclusiveListView.FullRowSelect = true;
			this.ExclusiveListView.GridLines = true;
			this.ExclusiveListView.Name = "ExclusiveListView";
			this.ExclusiveListView.UseCompatibleStateImageBehavior = false;
			this.ExclusiveListView.View = System.Windows.Forms.View.Details;
			this.ExclusiveListView.SelectedIndexChanged += new System.EventHandler( this.ExclusiveListView_SelectedIndexChanged );
			this.ExclusiveListView.DoubleClick += new System.EventHandler( this.ExclusiveListView_DoubleClick );
			this.ExclusiveListView.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler( this.ExclusiveListView_ColumnClick );
			// 
			// SizeInKByte
			// 
			resources.ApplyResources( this.SizeInKByte, "SizeInKByte" );
			// 
			// SizePercentage
			// 
			resources.ApplyResources( this.SizePercentage, "SizePercentage" );
			// 
			// Count
			// 
			resources.ApplyResources( this.Count, "Count" );
			// 
			// CountPercentage
			// 
			resources.ApplyResources( this.CountPercentage, "CountPercentage" );
			// 
			// GroupName
			// 
			resources.ApplyResources( this.GroupName, "GroupName" );
			// 
			// FunctionName
			// 
			resources.ApplyResources( this.FunctionName, "FunctionName" );
			// 
			// ExclusiveSingleCallStackView
			// 
			this.ExclusiveSingleCallStackView.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.ExclusiveSingleCallStackView.Columns.AddRange( new System.Windows.Forms.ColumnHeader[] {
            this.Function,
            this.File,
            this.Line} );
			resources.ApplyResources( this.ExclusiveSingleCallStackView, "ExclusiveSingleCallStackView" );
			this.ExclusiveSingleCallStackView.FullRowSelect = true;
			this.ExclusiveSingleCallStackView.GridLines = true;
			this.ExclusiveSingleCallStackView.Name = "ExclusiveSingleCallStackView";
			this.ExclusiveSingleCallStackView.UseCompatibleStateImageBehavior = false;
			this.ExclusiveSingleCallStackView.View = System.Windows.Forms.View.Details;
			// 
			// Function
			// 
			resources.ApplyResources( this.Function, "Function" );
			// 
			// File
			// 
			resources.ApplyResources( this.File, "File" );
			// 
			// Line
			// 
			resources.ApplyResources( this.Line, "Line" );
			// 
			// TimelineTabPage
			// 
			this.TimelineTabPage.Controls.Add( this.TimeLineChart );
			resources.ApplyResources( this.TimelineTabPage, "TimelineTabPage" );
			this.TimelineTabPage.Name = "TimelineTabPage";
			this.TimelineTabPage.UseVisualStyleBackColor = true;
			// 
			// TimeLineChart
			// 
			resources.ApplyResources( this.TimeLineChart, "TimeLineChart" );
			this.TimeLineChart.BackColor = System.Drawing.SystemColors.Control;
			chartArea1.BackColor = System.Drawing.SystemColors.Control;
			chartArea1.Name = "ChartArea1";
			this.TimeLineChart.ChartAreas.Add( chartArea1 );
			legend1.BackColor = System.Drawing.SystemColors.Control;
			legend1.Name = "Legend1";
			this.TimeLineChart.Legends.Add( legend1 );
			this.TimeLineChart.Name = "TimeLineChart";
			this.TimeLineChart.Palette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.Bright;
			series1.ChartArea = "ChartArea1";
			series1.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series1.Legend = "Legend1";
			series1.Name = "Image Size";
			series2.ChartArea = "ChartArea1";
			series2.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series2.Legend = "Legend1";
			series2.Name = "OS Overhead";
			series3.ChartArea = "ChartArea1";
			series3.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series3.Legend = "Legend1";
			series3.Name = "Virtual Used";
			series4.ChartArea = "ChartArea1";
			series4.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series4.Legend = "Legend1";
			series4.Name = "Virtual Slack";
			series5.ChartArea = "ChartArea1";
			series5.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series5.Legend = "Legend1";
			series5.Name = "Virtual Waste";
			series6.ChartArea = "ChartArea1";
			series6.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series6.Legend = "Legend1";
			series6.Name = "Physical Used";
			series7.ChartArea = "ChartArea1";
			series7.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series7.Legend = "Legend1";
			series7.Name = "Physical Slack";
			series8.ChartArea = "ChartArea1";
			series8.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StackedArea;
			series8.Legend = "Legend1";
			series8.Name = "Physical Waste";
			series9.ChartArea = "ChartArea1";
			series9.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
			series9.Color = System.Drawing.Color.White;
			series9.Legend = "Legend1";
			series9.Name = "Allocated Memory";
			this.TimeLineChart.Series.Add( series1 );
			this.TimeLineChart.Series.Add( series2 );
			this.TimeLineChart.Series.Add( series3 );
			this.TimeLineChart.Series.Add( series4 );
			this.TimeLineChart.Series.Add( series5 );
			this.TimeLineChart.Series.Add( series6 );
			this.TimeLineChart.Series.Add( series7 );
			this.TimeLineChart.Series.Add( series8 );
			this.TimeLineChart.Series.Add( series9 );
			// 
			// GroupContextMenu
			// 
			this.GroupContextMenu.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.GroupMenuItem,
            this.UngroupMenuItem} );
			this.GroupContextMenu.Name = "contextMenuStrip1";
			resources.ApplyResources( this.GroupContextMenu, "GroupContextMenu" );
			// 
			// GroupMenuItem
			// 
			this.GroupMenuItem.Name = "GroupMenuItem";
			resources.ApplyResources( this.GroupMenuItem, "GroupMenuItem" );
			// 
			// UngroupMenuItem
			// 
			this.UngroupMenuItem.Name = "UngroupMenuItem";
			resources.ApplyResources( this.UngroupMenuItem, "UngroupMenuItem" );
			// 
			// MainWindow
			// 
			resources.ApplyResources( this, "$this" );
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add( this.MainTabControl );
			this.Controls.Add( this.MainStatusStrip );
			this.Controls.Add( this.ToolStrip );
			this.Controls.Add( this.MainMenu );
			this.MainMenuStrip = this.MainMenu;
			this.Name = "MainWindow";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler( this.MainWindowFormClosing );
			this.MainMenu.ResumeLayout( false );
			this.MainMenu.PerformLayout();
			this.ToolStrip.ResumeLayout( false );
			this.ToolStrip.PerformLayout();
			this.MainStatusStrip.ResumeLayout( false );
			this.MainStatusStrip.PerformLayout();
			this.MainTabControl.ResumeLayout( false );
			this.CallGraphTabPage.ResumeLayout( false );
			this.CallGraphTabPage.PerformLayout();
			this.ExclusiveTabPage.ResumeLayout( false );
			this.ExclusiveSplitContainer.Panel1.ResumeLayout( false );
			this.ExclusiveSplitContainer.Panel2.ResumeLayout( false );
			this.ExclusiveSplitContainer.ResumeLayout( false );
			this.TimelineTabPage.ResumeLayout( false );
			( ( System.ComponentModel.ISupportInitialize )( this.TimeLineChart ) ).EndInit();
			this.GroupContextMenu.ResumeLayout( false );
			this.ResumeLayout( false );
			this.PerformLayout();

        }

        #endregion

		private System.Windows.Forms.TreeView CallGraphTreeView;
		private System.Windows.Forms.MenuStrip MainMenu;
		private System.Windows.Forms.ToolStripMenuItem FileMenuItem;
		private System.Windows.Forms.ToolStripMenuItem OpenMenuItem;
		private System.Windows.Forms.ToolStripMenuItem ExportToCSVMenuItem;
		private System.Windows.Forms.ToolStrip ToolStrip;
		private System.Windows.Forms.ToolStripLabel DiffStartToolLabel;
		private System.Windows.Forms.ToolStripComboBox DiffStartComboBox;
		private System.Windows.Forms.ToolStripLabel DiffEndToolLabel;
		private System.Windows.Forms.ToolStripComboBox DiffEndComboBox;
		private System.Windows.Forms.ToolStripButton GoButton;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
		private System.Windows.Forms.StatusStrip MainStatusStrip;
		private System.Windows.Forms.ToolStripStatusLabel StatusStripLabel;
		private System.Windows.Forms.TabControl MainTabControl;
		private System.Windows.Forms.TabPage CallGraphTabPage;
		private System.Windows.Forms.TabPage ExclusiveTabPage;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
		private System.Windows.Forms.ToolStripMenuItem HelpMenuItem;
        private System.Windows.Forms.ToolStripMenuItem QuickStartMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
		private System.Windows.Forms.Label ParentLabel;
		private System.Windows.Forms.Button ResetParentButton;
		private System.Windows.Forms.ToolStripLabel FilterLabel;
        private System.Windows.Forms.ToolStripTextBox FilterTextBox;
        private System.Windows.Forms.ContextMenuStrip GroupContextMenu;
        private System.Windows.Forms.ToolStripMenuItem GroupMenuItem;
        private System.Windows.Forms.ToolStripMenuItem UngroupMenuItem;
        private System.Windows.Forms.ToolStripStatusLabel SelectionSizeStatusLabel;
        private System.Windows.Forms.SplitContainer ExclusiveSplitContainer;
        private System.Windows.Forms.ListView ExclusiveListView;
        private System.Windows.Forms.ColumnHeader SizeInKByte;
        private System.Windows.Forms.ColumnHeader SizePercentage;
        private System.Windows.Forms.ColumnHeader Count;
        private System.Windows.Forms.ColumnHeader CountPercentage;
		private System.Windows.Forms.ColumnHeader GroupName;
        private System.Windows.Forms.ListView ExclusiveSingleCallStackView;
        private System.Windows.Forms.ColumnHeader Function;
        private System.Windows.Forms.ColumnHeader File;
		private System.Windows.Forms.ColumnHeader Line;
		private System.Windows.Forms.ToolStripMenuItem ToolMenuItem;
		private System.Windows.Forms.ToolStripMenuItem OptionsMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
		private System.Windows.Forms.ToolStripDropDownButton FilterClassesDropDownButton;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator8;
		private System.Windows.Forms.ToolStripSplitButton SortCriteriaSplitButton;
		private System.Windows.Forms.ToolStripMenuItem SortBySizeMenuItem;
		private System.Windows.Forms.ToolStripMenuItem SortByCountMenuItem;
		private System.Windows.Forms.ToolStripSplitButton AllocTypeSplitButton;
		private System.Windows.Forms.ToolStripMenuItem ActiveAllocsMenuItem;
		private System.Windows.Forms.ToolStripMenuItem LifetimeAllocsMenuItem;
		private System.Windows.Forms.ToolStripMenuItem HideTemplatesMenuItem;
		public System.Windows.Forms.ToolStripSplitButton ContainersSplitButton;
		private System.Windows.Forms.ToolStripMenuItem ShowTemplatesMenuItem;
		private System.Windows.Forms.ToolStripMenuItem FilterOutClassesMenuItem;
		private System.Windows.Forms.ToolStripMenuItem FilterInClassesMenuItem;
		private System.Windows.Forms.ToolStripSeparator toolStripSeparator9;
		public System.Windows.Forms.ToolStripSplitButton FilterTypeSplitButton;
		private System.Windows.Forms.ColumnHeader FunctionName;
		private System.Windows.Forms.TabPage TimelineTabPage;
		private System.Windows.Forms.DataVisualization.Charting.Chart TimeLineChart;
    }
}

