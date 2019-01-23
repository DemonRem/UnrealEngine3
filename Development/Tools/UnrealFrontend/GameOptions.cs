using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Windows.Forms;
using System.Text;
using System.Text.RegularExpressions;


namespace UnrealFrontend
{
	#region Game option classes
	// base option class, mainly for serialization, etc
	public abstract class GameOption
	{
		public string OptionName;

		// Saves the option value
		private string SavedValueString = "";

		public enum EOptionType
		{
			OT_URL,
			OT_CMDLINE,
			OT_EXEC,
			OT_CUSTOM,
		};
		public EOptionType Type;

		public GameOption(string InOptionName, EOptionType InType)
		{
			OptionName = InOptionName;
			Type = InType;
		}

		abstract public void LoadFromString(string StringValue);
		abstract public string WriteToString();
		public void WriteToString(out string StringValue)
		{
			StringValue = WriteToString();
		}

		/// <summary>
		/// Stores the option's value to a cached string for later update (ie, save the 
		/// value of a control bound by the option)
		/// </summary>
		public void SaveValue()
		{
			SavedValueString = WriteToString();
		}

		/// <summary>
		/// Restores the value of the option, eg to the bound control
		/// </summary>
		public void RestoreValue()
		{
			LoadFromString(SavedValueString);
		}

		virtual public string GetCommandlineOption()
		{
			// default is to return nothing
			return "";
		}

		virtual public void WriteExecOptions(StreamWriter Writer)
		{
			// do nothing
		}
	}

	public class SimpleIntGameOption : GameOption
	{
		public int Value;

		public SimpleIntGameOption(string InOptionName, int InValue)
			: base(InOptionName, EOptionType.OT_CUSTOM)
		{
			Value = InValue;
		}

		public override void LoadFromString(string StringValue)
		{
			Value = Convert.ToInt32(StringValue);
		}

		public override string WriteToString()
		{
			return Value.ToString();
		}
	}

	public class CheckBoxGameOption : GameOption
	{
		protected CheckBox Control;
		protected string TrueCmdLine;
		protected string FalseCmdLine;

		public CheckBoxGameOption(CheckBox InCheckBox, GameOption.EOptionType InType, string InOptionName, string InTrueCmdLine, string InFalseCmdLine, bool DefaultChecked)
			: base(InOptionName, InType)
		{
			// keep a ref to the control
			Control = InCheckBox;
			TrueCmdLine = InTrueCmdLine;
			FalseCmdLine = InFalseCmdLine;
			if (Control != null)
			{
				Control.Checked = DefaultChecked;
			}
		}

		override public void LoadFromString(string StringValue)
		{
			int Value = Convert.ToInt32(StringValue);
			// set the checked value based on the read value
			if (Control != null)
			{
				Control.Checked = (Value != 0);
			}
		}

		override public string WriteToString()
		{
			if (Control != null && Control.Checked)
			{
				return "1";
			}
			return "0";
		}

		override public string GetCommandlineOption()
		{
			if (Control != null && Control.Enabled)
			{
				if (Control.Checked)
				{
					return TrueCmdLine;
				}
				else
				{
					return FalseCmdLine;
				}
			}
			return base.GetCommandlineOption();
		}
	}

	public class ComboBoxGameOption : GameOption
	{
		protected ComboBox Control;

		public ComboBoxGameOption(ComboBox InComboBox, string InOptionName, string[] InValues)
			: base(InOptionName, EOptionType.OT_CUSTOM)
		{
			Control = InComboBox;
			// set the passed in contents
			if (Control != null && InValues != null)
			{
				foreach (string Value in InValues)
				{
					Control.Items.Add(Value);
				}
				Control.Text = Control.Items[0].ToString();
			}
		}

		public ComboBoxGameOption(ComboBox InComboBox, string InOptionName, EOptionType InType)
			: base(InOptionName, InType)
		{
			Control = InComboBox;
		}

		override public void LoadFromString(string StringValue)
		{
			Regex ValueRegex = new Regex("\\[([^\\[\\]]*)\\](.+)?");
			Match ValueMatch = null;
			while ((ValueMatch = ValueRegex.Match(StringValue)).Success)
			{
				string Value = ValueMatch.Groups[1].Value.ToString();
				if (Control != null)
				{
					// search for an existing entry
					if (!Control.Items.Contains(Value))
					{
						// add it now
						Control.Items.Add(Value);
					}
					// select the last entry read
					Control.Text = Value;
				}
				// if there are options after this one
				if (ValueMatch.Groups.Count > 1)
				{
					// set the string to the remaining and attempt to match again
					StringValue = ValueMatch.Groups[2].Value.ToString();
				}
				else
				{
					// no need to keep looking for options
					break;
				}
			}
		}

		override public string WriteToString()
		{
			string StringValue = "";
			if (Control != null)
			{
				for (int Index = 0; Index < Control.Items.Count; Index++)
				{
					string Value = Control.Items[Index].ToString();
					// skip the currently selected since it'll be on the end
					if (Value != Control.Text)
					{
						StringValue += "[" + Value + "]";
					}
				}
				StringValue += "[" + Control.Text + "]";
			}
			return StringValue;
		}

		override public string GetCommandlineOption()
		{
			if (Control != null && Control.Enabled)
			{
				return Control.Text;
			}
			return base.GetCommandlineOption();
		}
	}

	public class ResolutionGameOption : ComboBoxGameOption
	{
		public ResolutionGameOption(ComboBox InComboBox, string InOptionName, string[] InValues)
			: base(InComboBox, InOptionName, InValues)
		{
			// force the type to command line option
			Type = EOptionType.OT_CMDLINE;
		}

		public override string GetCommandlineOption()
		{
			if (Control != null && Control.Enabled)
			{
				Regex ResolutionRegex = new Regex("(\\d+)x(\\d+)");
				Match ResolutionMatch = ResolutionRegex.Match(Control.Text);
				if (ResolutionMatch.Success)
				{
					return " -resX=" + ResolutionMatch.Groups[1].Value.ToString() + " -resY=" + ResolutionMatch.Groups[2].Value.ToString();
				}
			}
			return base.GetCommandlineOption();
		}
	}

	public class TextBoxGameOption : GameOption
	{
		TextBox Control;

		public TextBoxGameOption(TextBox InTextBox, string InOptionName, EOptionType InType)
			: base(InOptionName, InType)
		{
			Control = InTextBox;
		}

		public override void LoadFromString(string StringValue)
		{
			Regex ValueRegex = new Regex("\\[([^\\[\\]]+)\\](.+)?");
			Match ValueMatch = null;
			ArrayList Values = new ArrayList();
			while ((ValueMatch = ValueRegex.Match(StringValue)).Success)
			{
				string Value = ValueMatch.Groups[1].Value.ToString();
				Values.Add(Value);
				// if there are options after this one
				if (ValueMatch.Groups.Count > 1)
				{
					// set the string to the remaining and attempt to match again
					StringValue = ValueMatch.Groups[2].Value.ToString();
				}
				else
				{
					// no need to keep looking for options
					break;
				}
			}
			if (Control != null)
			{
				Control.Lines = (string[])Values.ToArray(typeof(string));
			}
		}

		public override string WriteToString()
		{
			string StringValue = "";
			if (Control != null)
			{
				foreach (string Line in Control.Lines)
				{
					StringValue += "[" + Line + "]";
				}
			}
			return StringValue;
		}

		override public void WriteExecOptions(StreamWriter Writer)
		{
			if (Control != null)
			{
				foreach (string Line in Control.Lines)
				{
					Writer.WriteLine(Line);
				}
			}
		}
	}
	#endregion

}