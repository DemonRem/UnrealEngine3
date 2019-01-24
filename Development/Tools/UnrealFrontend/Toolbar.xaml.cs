/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace UnrealFrontend
{
	/// <summary>
	/// Interaction logic for Toolbar.xaml
	/// </summary>
	public partial class Toolbar : UserControl
	{
		public Toolbar()
		{
			InitializeComponent();
		}

		void OnStartProcess(object sender, RoutedEventArgs e)
		{
			if (IsWorking)
			{
				Session.Current.StopProcess();
			}
			else
			{
				Session.Current.RunProfile(CurrentProfile, Pipeline.ERunOptions.None);
				mStartCombo.OnMenuItemAction();
			}
		}


		void OnRebuildScript(object sender, RoutedEventArgs e)
		{
			Session.Current.RunProfile(CurrentProfile, Pipeline.ERunOptions.RebuildScript);
			mStartCombo.OnMenuItemAction();
		}

		void OnCookINIsOnly(object sender, RoutedEventArgs e)
		{
			Session.Current.RunProfile(CurrentProfile, Pipeline.ERunOptions.CookINIsOnly);
			mStartCombo.OnMenuItemAction();
		}

		void OnFullRecook(object sender, RoutedEventArgs e)
		{
			Session.Current.RunProfile(CurrentProfile, Pipeline.ERunOptions.FullReCook);
			mStartCombo.OnMenuItemAction();
		}

		void OnFullRebuildAndRecook(object sender, RoutedEventArgs e)
		{
			Session.Current.RunProfile(CurrentProfile, Pipeline.ERunOptions.RebuildScript | Pipeline.ERunOptions.FullReCook);
			mStartCombo.OnMenuItemAction();
		}

		void OnRebootTargets(object sender, RoutedEventArgs e)
		{
			Session.Current.QueueReboot(CurrentProfile);
		}

		void OnLaunchConsole(object sender, RoutedEventArgs e)
		{
			Session.Current.QueueLaunchUnrealConsole(CurrentProfile);
		}

		public MainWindow MainWindow
		{
			get { return (MainWindow)GetValue(MainWindowProperty); }
			set { SetValue(MainWindowProperty, value); }
		}
		public static readonly DependencyProperty MainWindowProperty =
			DependencyProperty.Register("MainWindow", typeof(MainWindow), typeof(Toolbar), new UIPropertyMetadata(null));



		/// <summary>
		/// The currenly selected profile.
		/// </summary>
		public Profile CurrentProfile
		{
			get { return (Profile)GetValue(CurrentProfileProperty); }
			set { SetValue(CurrentProfileProperty, value); }
		}
		public static readonly DependencyProperty CurrentProfileProperty =
			DependencyProperty.Register("CurrentProfile", typeof(Profile), typeof(Toolbar), new UIPropertyMetadata(null));


		/// <summary>
		/// A convenience property that is true when the app is busy.
		/// </summary>
		public bool IsWorking
		{
			get { return (bool)GetValue(IsWorkingProperty); }
			set { SetValue(IsWorkingProperty, value); }
		}
		public static readonly DependencyProperty IsWorkingProperty =
			DependencyProperty.Register("IsWorking", typeof(bool), typeof(Toolbar), new UIPropertyMetadata(null));
	}
}
