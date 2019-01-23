/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Windows;
using System.Windows.Data;

namespace UnrealFrontend
{
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
	}

	/// <summary>
	/// Converts a negated bool to either Visible(false) or Collapsed(true)
	/// </summary>
	[ValueConversion(typeof(bool), typeof(Visibility))]
	public class NegatedBooleanToVisibilityConverter
		: IValueConverter
	{
		/// Converts from the source type to the target type
		public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			bool Val = (bool)value;
			return (Val == true ? Visibility.Collapsed : Visibility.Visible);
		}

		/// Converts back to the source type from the target type
		public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			return null;	// Not supported
		}
	}

	/// <summary>
	/// Converts a bool to either Visible(true) or Hidden(false)
	/// </summary>
	[ValueConversion(typeof(bool), typeof(Visibility))]
	public class BooleanToVisibilityConverter_Hidden
		: IValueConverter
	{
		/// Converts from the source type to the target type
		public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			bool Val = (bool)value;
			return (Val == true ? Visibility.Visible : Visibility.Hidden);
		}

		/// Converts back to the source type from the target type
		public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			return null;	// Not supported
		}
	}

	/// <summary>
	/// Converts a bool to 100% opacity (true) or partial opacity (false)
	/// </summary>
	[ValueConversion(typeof(bool), typeof(Visibility))]
	public class BooleanToOpacityConverter
		: IValueConverter
	{
		/// Converts from the source type to the target type
		public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			bool Val = (bool)value;
			return (Val == true ? 1.0 : 0.25);
		}

		/// Converts back to the source type from the target type
		public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			return null;	// Not supported
		}
	}

	/// <summary>
	/// Converts a boolean to its opposite value
	/// </summary>
	[ValueConversion(typeof(Boolean), typeof(Boolean))]
	public class NegatingConverter
		: IValueConverter
	{
		/// Converts from the source type to the target type
		public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			Boolean BoolValue = (Boolean)value;
			return !BoolValue;
		}

		/// Converts back to the source type from the target type
		public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			Boolean BoolValue = (Boolean)value;
			return !BoolValue;
		}
	}

	/// <summary>
	/// Converts any non-null object to true; null to false.
	/// </summary>
	[ValueConversion(typeof(object), typeof(bool))]
	public class IsNotNullToBoolConverter
		: IValueConverter
	{
		/// Converts from the source type to the target type
		public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			return (value != null);
		}

		/// Converts back to the source type from the target type
		public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			return null;	// Not supported
		}
	}

	/// <summary>
	/// Converts the package mode enum to a friendly string representation.
	/// </summary>
	[ValueConversion(typeof(Pipeline.EPackageMode), typeof(String))]
	public class MobilePackageModeToString
		: IValueConverter
	{
		/// Converts from the source type to the target type
		public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			try
			{
				Pipeline.EPackageMode PackageMode = (Pipeline.EPackageMode)value;
				return Pipeline.PackageIOS.EPackageMode_ToFriendlyName(PackageMode);
			}
			catch(Exception)
			{
				return "Unrecognized Package Mode";
			}
			
		}

		/// Converts back to the source type from the target type
		public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
		{
			return null;	// Not supported
		}
	}

}
