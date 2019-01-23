/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEdCLR.h"
#include "ConvertersCLR.h"
#include "ManagedCodeSupportCLR.h"
#include "LandscapeEditWindowShared.h"
#include "WPFWindowWrapperCLR.h"
#include "WPFFrameCLR.h"

using namespace System::Windows::Controls::Primitives;
using namespace System::Windows::Media::Imaging;
using namespace System::Windows::Input;
using namespace System::Deployment;
using namespace WPF_Landscape;

#pragma unmanaged
#include "LandscapeEdMode.h"
#if WITH_EDITOR
#include "LandscapeRender.h"
#endif
#pragma managed

#define DECLARE_LANDSCAPE_PROPERTY( InPropertyType, InPropertyName ) \
		property InPropertyType InPropertyName \
		{ \
		InPropertyType get() { return LandscapeEditSystem->UISettings.Get##InPropertyName(); } \
			void set( InPropertyType Value ) \
			{ \
				if( LandscapeEditSystem->UISettings.Get##InPropertyName() != Value ) \
				{ \
					LandscapeEditSystem->UISettings.Set##InPropertyName(Value); \
				} \
			} \
		}

#define DECLARE_ENUM_LANDSCAPE_PROPERTY( InPropertyType, InPropertyName, EnumType, EnumVariable, EnumValue ) \
		property InPropertyType InPropertyName \
		{ \
			InPropertyType get() { return (LandscapeEditSystem->UISettings.Get##EnumVariable() == EnumValue ? true : false); } \
			void set( InPropertyType Value ) \
			{ \
				if( Value != InPropertyName ) \
				{ \
					LandscapeEditSystem->UISettings.Set##EnumVariable( (Value == true ? EnumValue : (EnumType)-1) ); \
				} \
			} \
		}

/**
 * Landscape Edit window control (managed)
 */
ref class MLandscapeEditWindow
	: public MWPFWindowWrapper,
	  public ComponentModel::INotifyPropertyChanged

{

private:
	ComboBox^ LandscapeComboBox;
	Button^ ImportButton;

	// Landscapes
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, CurrentLandscapeIndex);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(List<String^>^, Landscapes);

	// Import
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(String^, HeightmapFileNameString);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, Width);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, Height);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, SectionSize);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, NumSections);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, TotalComponents);
	LandscapeImportLayers^ LandscapeImportLayersValue;
	DECLARE_MAPPED_NOTIFY_PROPERTY(LandscapeImportLayers^, LandscapeImportLayersProperty, LandscapeImportLayers^, LandscapeImportLayersValue)

	// Export
	UBOOL bExportLayers;
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( ExportLayers, bExportLayers );

	// Targets
	LandscapeTargets^ LandscapeTargetsValue;
	ListBox^ TargetListBox;
	DECLARE_MAPPED_NOTIFY_PROPERTY(LandscapeTargets^, LandscapeTargetsProperty, LandscapeTargets^, LandscapeTargetsValue)
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(INT, CurrentTargetIndex);

	// Add/Remove...
	UBOOL bNoBlending;
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(String^, AddLayerNameString);
	DECLARE_SIMPLE_NOTIFY_PROPERTY_AND_VALUE(FLOAT, Hardness);
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( NoBlending, bNoBlending );

	// Tools/Brushes
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, ToolStrength);
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, BrushRadius);
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, BrushFalloff);
	// Erode Tool
	DECLARE_LANDSCAPE_PROPERTY(INT, ErodeThresh);
	DECLARE_LANDSCAPE_PROPERTY(INT, ErodeIterationNum);
	DECLARE_LANDSCAPE_PROPERTY(INT, ErodeSurfaceThickness);
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsErosionNoiseModeBoth, ELandscapeToolNoiseMode::Type, ErosionNoiseMode, ELandscapeToolNoiseMode::Both );
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsErosionNoiseModeAdd, ELandscapeToolNoiseMode::Type, ErosionNoiseMode, ELandscapeToolNoiseMode::Add );
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsErosionNoiseModeSub, ELandscapeToolNoiseMode::Type, ErosionNoiseMode, ELandscapeToolNoiseMode::Sub );
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, ErosionNoiseScale);
	// Hydra Erosion Tool
	DECLARE_LANDSCAPE_PROPERTY(INT, RainAmount);
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, SedimentCapacity);
	DECLARE_LANDSCAPE_PROPERTY(INT, HErodeIterationNum);
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsRainDistModeBoth, ELandscapeToolNoiseMode::Type, RainDistMode, ELandscapeToolNoiseMode::Both );
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsRainDistModeAdd, ELandscapeToolNoiseMode::Type, RainDistMode, ELandscapeToolNoiseMode::Add );
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, RainDistScale);
	DECLARE_LANDSCAPE_PROPERTY(UBOOL, bHErosionDetailSmooth);
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, HErosionDetailScale);
	// Noise Tool
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsNoiseModeBoth, ELandscapeToolNoiseMode::Type, NoiseMode, ELandscapeToolNoiseMode::Both );
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsNoiseModeAdd, ELandscapeToolNoiseMode::Type, NoiseMode, ELandscapeToolNoiseMode::Add );
	DECLARE_ENUM_LANDSCAPE_PROPERTY( bool, IsNoiseModeSub, ELandscapeToolNoiseMode::Type, NoiseMode, ELandscapeToolNoiseMode::Sub );
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, NoiseScale);
	// Detail Preserving Smooth
	DECLARE_LANDSCAPE_PROPERTY(UBOOL, bDetailSmooth);
	DECLARE_LANDSCAPE_PROPERTY(FLOAT, DetailScale);

	// ViewMode
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsViewModeNormal, ELandscapeViewMode::Type, GLandscapeViewMode, ELandscapeViewMode::Normal );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsViewModeEditLayer, ELandscapeViewMode::Type, GLandscapeViewMode, ELandscapeViewMode::EditLayer );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsViewModeDebugLayer, ELandscapeViewMode::Type, GLandscapeViewMode, ELandscapeViewMode::DebugLayer );

	INT HeightmapFileSize;
	String^ LastImportPath;

public:

	// tor
	MLandscapeEditWindow( FEdModeLandscape* InLandscapeEditSystem)
	:	LandscapeEditSystem(InLandscapeEditSystem)
	,	bExportLayers(FALSE)
	,	bNoBlending(FALSE)
	{
		check( InLandscapeEditSystem != NULL );
		Hardness = 0.5f;
	}

	/**
	 * Initialize the landscape edit window
	 *
	 * @param	InLandscapeEditSystem	Landscape edit system that owns us
	 * @param	InParentWindowHandle	Parent window handle
	 *
	 * @return	TRUE if successful
	 */
	UBOOL InitLandscapeEditWindow( const HWND InParentWindowHandle )
	{
		String^ WindowTitle = CLRTools::LocalizeString( "LandscapeEditWindow_WindowTitle" );
		String^ WPFXamlFileName = "LandscapeEditWindow.xaml";

		HeightmapFileSize = INDEX_NONE;
		LastImportPath = CLRTools::ToString(GApp->LastDir[LD_GENERIC_IMPORT]);


		// We draw our own title bar so tell the window about it's height
		const int FakeTitleBarHeight = 28;
		const UBOOL bIsTopMost = FALSE;


		// Read the saved size/position
		INT x,y,w,h;
		LandscapeEditSystem->UISettings.GetWindowSizePos(x,y,w,h);

		// If we don't have an initial position yet then default to centering the new window
		bool bCenterWindow = x == -1 || y == -1;

		// Call parent implementation's init function to create the actual window
		if( !InitWindow( InParentWindowHandle,
						 WindowTitle,
						 WPFXamlFileName,
						 x,
						 y,
						 w,
						 h,
						 bCenterWindow,
						 FakeTitleBarHeight,
						 bIsTopMost ) )
		{
			return FALSE;
		}

		// Reset the active landscape in the EdMode so the initial combo box setting propagates to the layers correctly.
		LandscapeEditSystem->CurrentToolTarget.Landscape = NULL;


		// Register for property change callbacks from our properties object
		this->PropertyChanged += gcnew ComponentModel::PropertyChangedEventHandler( this, &MLandscapeEditWindow::OnLandscapeEditPropertyChanged );

		// Setup bindings
		Visual^ RootVisual = InteropWindow->RootVisual;

		FrameworkElement^ WindowContentElement = safe_cast<FrameworkElement^>(RootVisual);
		WindowContentElement->DataContext = this;


		// Lookup any items we hold onto first, to avoid crashes in OnPropertyChanged
		LandscapeComboBox = safe_cast< ComboBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "LandscapeCombo" ) );
		ImportButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ImportButton" ) );

		//
		// Editing items
		//
		UnrealEd::Utils::CreateBinding(LandscapeComboBox, ComboBox::SelectedIndexProperty, this, "CurrentLandscapeIndex", gcnew IntToIntOffsetConverter( 0 ) );
		UnrealEd::Utils::CreateBinding(LandscapeComboBox, ComboBox::ItemsSourceProperty, this, "Landscapes");


		// To add 'Tool' buttons
		UniformGrid^ ToolGrid = safe_cast< UniformGrid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ToolGrid" ) );
		ToolGrid->Columns = 4;
		for( INT ToolIdx=0;ToolIdx<LandscapeEditSystem->LandscapeToolSets.Num();ToolIdx++ )
		{
			CustomControls::ImageRadioButton^ btn = gcnew CustomControls::ImageRadioButton();
			ToolGrid->Children->Add(btn);
			btn->Click += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::ToolButton_Click );
			btn->CheckedImage = gcnew BitmapImage( gcnew Uri( gcnew System::String(*FString::Printf(TEXT("%swxres\\Landscape_Tool_%s_active.png"), *GetEditorResourcesDir(),
				LandscapeEditSystem->LandscapeToolSets(ToolIdx)->GetIconString())), UriKind::Absolute) );
			btn->UncheckedImage = gcnew BitmapImage( gcnew Uri( gcnew System::String(*FString::Printf(TEXT("%swxres\\Landscape_Tool_%s_inactive.png"), *GetEditorResourcesDir(),
				LandscapeEditSystem->LandscapeToolSets(ToolIdx)->GetIconString())), UriKind::Absolute) );
			btn->ToolTip = gcnew System::String(*LandscapeEditSystem->LandscapeToolSets(ToolIdx)->GetTooltipString());
		}
		// Tool properties
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ToolStrengthSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "ToolStrength" );
		
		StackPanel^ ErosionPanel = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErosionPanel" ) );
		// Erode Tool properties
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErodeThreshSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "ErodeThresh" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErodeIterationNumSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "ErodeIterationNum" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErodeSurfaceThicknessSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "ErodeSurfaceThickness" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErosionNoiseModeBoth" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsErosionNoiseModeBoth" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErosionNoiseModeAdd" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsErosionNoiseModeAdd" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErosionNoiseModeSub" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsErosionNoiseModeSub" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErosionNoiseScaleSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "ErosionNoiseScale" );
		ErosionPanel->Visibility = Visibility::Collapsed;

		// Hydraulic Erode Tool properties
		StackPanel^ HydraErosionPanel = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HydraulicErosionPanel" ) );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "RainAmountSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "RainAmount" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "SedimentCapacitySlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "SedimentCapacity" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HErodeIterationNumSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "HErodeIterationNum" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "RainDistModeBoth" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsRainDistModeBoth" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "RainDistModeAdd" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsRainDistModeAdd" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "RainDistScaleSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "RainDistScale" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HErosoinDetailSmoothCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "bHErosionDetailSmooth" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^  >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HErosionDetailScaleSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "HErosionDetailScale" );
		HydraErosionPanel->Visibility = Visibility::Collapsed;

		// Noise Tool properties
		StackPanel^ NoisePanel = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NoisePanel" ) );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NoiseModeBoth" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsNoiseModeBoth" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NoiseModeAdd" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsNoiseModeAdd" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NoiseModeSub" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsNoiseModeSub" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NoiseScaleSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "NoiseScale" );
		NoisePanel->Visibility = Visibility::Collapsed;

		// Detail Preserving Smooth Tool properties
		StackPanel^ SmoothOptionPanel = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "SmoothOptionPanel" ) );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "DetailSmoothCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "bDetailSmooth" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^  >( LogicalTreeHelper::FindLogicalNode( RootVisual, "DetailScaleSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "DetailScale" );
		SmoothOptionPanel->Visibility = Visibility::Collapsed;

		// Layer View Mode
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EditmodeNone" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsViewModeNormal" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EditmodeEdit" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsViewModeEditLayer" );

		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EditmodeDebugView" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsViewModeDebugLayer" );

		// Brushes
		UniformGrid^ BrushGrid = safe_cast< UniformGrid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushGrid" ) );

		for( INT BrushSetIdx=0;BrushSetIdx<LandscapeEditSystem->LandscapeBrushSets.Num();BrushSetIdx++ )
		{
			FLandscapeBrushSet& BrushSet = LandscapeEditSystem->LandscapeBrushSets(BrushSetIdx);

			CustomControls::ToolDropdownRadioButton^ btn = gcnew CustomControls::ToolDropdownRadioButton();
			btn->ToolTip = gcnew System::String(*BrushSet.ToolTip);
			for( INT BrushIdx=0;BrushIdx < BrushSet.Brushes.Num();BrushIdx++ )
			{
				BitmapImage^ ImageUnchecked = gcnew BitmapImage( gcnew Uri( gcnew System::String(*FString::Printf(TEXT("%swxres\\Landscape_Brush_%s_inactive.png"), *GetEditorResourcesDir(), BrushSet.Brushes(BrushIdx)->GetIconString())), UriKind::Absolute) );
				BitmapImage^ ImageChecked = gcnew BitmapImage( gcnew Uri( gcnew System::String(*FString::Printf(TEXT("%swxres\\Landscape_Brush_%s_active.png"), *GetEditorResourcesDir(), BrushSet.Brushes(BrushIdx)->GetIconString())), UriKind::Absolute) );
				btn->ListItems->Add( gcnew CustomControls::ToolDropdownItem(ImageUnchecked, ImageChecked, gcnew System::String(*BrushSet.Brushes(BrushIdx)->GetTooltipString())) );
				btn->ToolSelectionChanged += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::BrushButton_SelectionChanged );

				// Fallback in case this brush is not the currently selected one
				if( BrushIdx == 0 )
				{
					btn->CheckedImage = ImageChecked;
					btn->UncheckedImage = ImageUnchecked;
					btn->SelectedIndex = 0;
				}

				if( BrushSet.Brushes(BrushIdx) == LandscapeEditSystem->CurrentBrush )
				{
					btn->IsChecked = TRUE;
					btn->CheckedImage = ImageChecked;
					btn->UncheckedImage = ImageUnchecked;
					btn->SelectedIndex = BrushIdx;
				}
			}
			BrushGrid->Children->Add(btn);
		}

		// Brush properties
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushRadiusSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "BrushRadius" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushFalloffSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "BrushFalloff" );

		//
		// Target list
		//
		LandscapeTargetsValue = gcnew LandscapeTargets();
		TargetListBox = safe_cast< ListBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "TargetListBox" ) );
		UnrealEd::Utils::CreateBinding(TargetListBox, ListBox::ItemsSourceProperty, this, "LandscapeTargetsProperty");
		// Edit Target Layers...
		RoutedCommand^ EditTargetLayerCommand = safe_cast< RoutedCommand^ >(WindowContentElement->FindResource("LandscapeEditTargetLayerCommand"));
		WindowContentElement->CommandBindings->Add(
			gcnew CommandBinding(EditTargetLayerCommand, gcnew ExecutedRoutedEventHandler( this, &MLandscapeEditWindow::OnLandscapeTargetLayerChanged) ) );

		UnrealEd::Utils::CreateBinding(TargetListBox, ListBox::SelectedIndexProperty, this, "CurrentTargetIndex", gcnew IntToIntOffsetConverter( 0 ) );


		// Add Layer
		Button^ AddLayerButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "AddLayerButton" ) );
		AddLayerButton->Click += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::AddLayerButton_Click );

		TextBox^ AddLayerNameTextBox = safe_cast< TextBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "AddLayerNameTextBox" ) );
		AddLayerNameTextBox->KeyUp += gcnew KeyEventHandler( this, &MLandscapeEditWindow::OnKeyUp);

		TextBox^ AddHardnessTextBox = safe_cast< TextBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "AddHardnessTextBox" ) );
		AddHardnessTextBox->KeyUp += gcnew KeyEventHandler( this, &MLandscapeEditWindow::OnKeyUp);

		UnrealEd::Utils::CreateBinding(AddLayerNameTextBox, TextBox::TextProperty, this, "AddLayerNameString" );
		AddLayerNameString = ""; // this is to work around a strange crash.
		UnrealEd::Utils::CreateBinding(AddHardnessTextBox, TextBox::TextProperty, this, "Hardness" );

		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "AddNoBlendingCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "NoBlending" );

		// Remove Layer
		RoutedCommand^ RemoveLayerCommand = safe_cast< RoutedCommand^ >(WindowContentElement->FindResource("LandscapeTargetLayerRemoveCommand"));
		WindowContentElement->CommandBindings->Add(
			gcnew CommandBinding(RemoveLayerCommand, gcnew ExecutedRoutedEventHandler( this, &MLandscapeEditWindow::TargetLayerRemoveButton_Click) ) );


		// This can trigger change notifications
		UpdateLandscapeList();

		//
		// Import items
		//
		Button^ HeightmapFileButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HeightmapFileButton" ) );
		HeightmapFileButton->Click += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::HeightmapFileButton_Click );

		TextBox^ HeightmapFileNameTextBox = safe_cast< TextBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HeightmapFileNameTextBox" ) );
		HeightmapFileNameTextBox->KeyUp += gcnew KeyEventHandler( this, &MLandscapeEditWindow::OnKeyUp);

		TextBox^ WidthTextBox = safe_cast< TextBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "WidthTextBox" ) );
		WidthTextBox->KeyUp += gcnew KeyEventHandler( this, &MLandscapeEditWindow::OnKeyUp);

		TextBox^ HeightTextBox = safe_cast< TextBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HeightTextBox" ) );
		HeightTextBox->KeyUp += gcnew KeyEventHandler( this, &MLandscapeEditWindow::OnKeyUp);

		UnrealEd::Utils::CreateBinding(HeightmapFileNameTextBox, TextBox::TextProperty, this, "HeightmapFileNameString" );
		UnrealEd::Utils::CreateBinding(WidthTextBox, TextBox::TextProperty, this, "Width" );
		UnrealEd::Utils::CreateBinding(HeightTextBox, TextBox::TextProperty, this, "Height" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< ComboBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "SectionSizeCombo" ) ),
			ComboBox::SelectedIndexProperty, this, "SectionSize", gcnew IntToIntOffsetConverter( 0 ) );
		UnrealEd::Utils::CreateBinding(
			safe_cast< ComboBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NumSectionsCombo" ) ),
			ComboBox::SelectedIndexProperty, this, "NumSections", gcnew IntToIntOffsetConverter( 0 ) );
		UnrealEd::Utils::CreateBinding(
			safe_cast< TextBlock^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "TotalComponentsLabel" ) ),
			TextBlock::TextProperty, this, "TotalComponents" );

		LandscapeImportLayersValue = gcnew LandscapeImportLayers();
		ItemsControl^ ImportLayersItemsControl = safe_cast< ItemsControl^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ImportLayersItemsControl" ) );
		UnrealEd::Utils::CreateBinding(ImportLayersItemsControl, ItemsControl::ItemsSourceProperty, this, "LandscapeImportLayersProperty");

		RoutedCommand^ ImportLayersFilenameCommand = safe_cast< RoutedCommand^ >(WindowContentElement->FindResource("LandscapeImportLayersFilenameCommand"));
		WindowContentElement->CommandBindings->Add(
			gcnew CommandBinding(ImportLayersFilenameCommand, gcnew ExecutedRoutedEventHandler( this, &MLandscapeEditWindow::LayerFileButton_Click) ) );

		RoutedCommand^ ImportLayersRemoveCommand = safe_cast< RoutedCommand^ >(WindowContentElement->FindResource("LandscapeImportLayersRemoveCommand"));
		WindowContentElement->CommandBindings->Add(
			gcnew CommandBinding(ImportLayersRemoveCommand, gcnew ExecutedRoutedEventHandler( this, &MLandscapeEditWindow::LayerRemoveButton_Click) ) );

		ImportButton->Click += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::ImportButton_Click );
		ImportButton->IsEnabled = FALSE;

		//
		// Export
		//
		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ExportLayersCheckbox" ) ),
			CheckBox::IsCheckedProperty, this, "ExportLayers" );

		Button^ ExportButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ExportButton" ) );
		ExportButton->Click += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::ExportButton_Click );


		//
		// Convert Terrain
		//
		Button^ ConvertTerrainButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ConvertTerrainButton" ) );
		ConvertTerrainButton->Click += gcnew RoutedEventHandler( this, &MLandscapeEditWindow::ConvertTerrainButton_Click );

		// Show the window!
		ShowWindow( true );

		return TRUE;
	}

protected:

	/**Add ability to commit text ctrls on "ENTER" */
	void OnKeyUp(Object^ Owner, KeyEventArgs^ Args)
	{
		if (Args->Key == Key::Return)
		{
			TextBox^ ChangedTextBox = safe_cast< TextBox^ >(Args->Source);
			if (ChangedTextBox != nullptr)
			{
				ChangedTextBox->GetBindingExpression(ChangedTextBox->TextProperty)->UpdateSource();
			}
		}
	}

	/** Called when a landscape edit window property is changed */
	void OnLandscapeEditPropertyChanged( Object^ Owner, ComponentModel::PropertyChangedEventArgs^ Args )
	{
		UBOOL RecalcSizes = FALSE;
		if( String::Compare(Args->PropertyName, "HeightmapFileNameString")==0 )
		{
			HeightmapFileSize = GFileManager->FileSize( *CLRTools::ToFString(HeightmapFileNameString) );

			if( HeightmapFileSize == INDEX_NONE )
			{
				Width = 0;
				Height = 0;
				SectionSize = 0;
				NumSections = 0;
				TotalComponents = 0;
			}
			else
			{
				// Guess dimensions from filesize
				INT w = appTrunc(appSqrt((FLOAT)(HeightmapFileSize>>1)));
				INT h = (HeightmapFileSize>>1) / w;

				// Keep searching for the most squarelike size
				while( (w * h * 2) != HeightmapFileSize )
				{
					w--;
					if( w <= 0 )
					{
						w = 0;
						h = 0;
						SectionSize = 0;
						NumSections = 0;
						TotalComponents = 0;
						break;
					}
					h = (HeightmapFileSize>>1) / w;
				}
				Width = w;
				Height = h;
			}
			RecalcSizes = TRUE;
		}

		if( String::Compare(Args->PropertyName, "Width")==0 )
		{
			if( Width > 0 && HeightmapFileSize > 0)
			{
				INT h = (HeightmapFileSize>>1) / Width;
				if( (Width * h * 2) == HeightmapFileSize )
				{
					Height = h;
				}
			}
			RecalcSizes = TRUE;
		}

		if( String::Compare(Args->PropertyName, "Height")==0 )
		{
			if( Height > 0 && HeightmapFileSize > 0)
			{
				INT w = (HeightmapFileSize>>1) / Height;
				if( (w * Height * 2) == HeightmapFileSize )
				{
					Width = w;
				}
			}
			RecalcSizes = TRUE;
		}

		if( RecalcSizes )
		{
			TotalComponents = 0;
			UBOOL bFoundMatch = FALSE;
			if( Width > 0 && Height > 0 )
			{
				// Try to find a section size and number of sections that matches the dimensions of the heightfield
				for( INT SectionSizeItem=1;SectionSizeItem<7;SectionSizeItem++ )
				{
					for(INT NumSectionsItem=2;NumSectionsItem>=1;NumSectionsItem-- )
					{
						// ss is 255, 127, 63, or 31, 15 or 7 quads per section
						INT ss = (256 >> (SectionSizeItem-1)) - 1;
						// ns is 2 or 1
						INT ns = NumSectionsItem;

						if( ((Width-1) % (ss * ns)) == 0 &&
							((Height-1) % (ss * ns)) == 0 )
						{
							bFoundMatch = TRUE;
							// Update combo boxes
							SectionSize = SectionSizeItem;
							NumSections = NumSectionsItem;
							TotalComponents = (Width-1) * (Height-1) / Square(ss * ns);
							break;
						}					
					}
					if( bFoundMatch )
					{
						break;
					}
				}
				if( !bFoundMatch )
				{
					SectionSize = 0;
					NumSections = 0;
					TotalComponents = 0;
				}
			}
		}

		if( String::Compare(Args->PropertyName, "SectionSize")==0 )
		{
			UBOOL bFoundMatch = FALSE;
			if( SectionSize > 0 )
			{
				for(INT NumSectionsItem=2;NumSectionsItem>=1;NumSectionsItem-- )
				{
					// ss is 255, 127, 63, or 31, 15 or 7 quads per section
					INT ss = (256 >> (SectionSize-1)) - 1;
					// ns is 2 or 1
					INT ns = NumSectionsItem;

					if( ((Width-1) % (ss * ns)) == 0 &&
						((Height-1) % (ss * ns)) == 0 )
					{
						TotalComponents = (Width-1) * (Height-1) / Square(ss * ns);
						NumSections = NumSectionsItem;
						bFoundMatch = TRUE;
						break;
					}
				}
			}
			if( !bFoundMatch )
			{
				TotalComponents = 0;
				NumSections = 0;
			}
		}

		if( String::Compare(Args->PropertyName, "NumSections")==0 )
		{
			UBOOL bFoundMatch = FALSE;
			if( NumSections > 0 )
			{
				for( INT SectionSizeItem=1;SectionSizeItem<7;SectionSizeItem++ )
				{
					// ss is 255, 127, 63, or 31, 15 or 7 quads per section
					INT ss = (256 >> (SectionSizeItem-1)) - 1;
					// ns is 2 or 1
					INT ns = NumSections;

					if( ((Width-1) % (ss * ns)) == 0 &&
						((Height-1) % (ss * ns)) == 0 )
					{
						TotalComponents = (Width-1) * (Height-1) / Square(ss * ns);
						SectionSize = SectionSizeItem;
						bFoundMatch  = TRUE;
						break;
					}		
				}
			}
			if( !bFoundMatch  )
			{
				SectionSize = 0;
				TotalComponents = 0;
			}
		}

		INT SectionSizeQuads = (256 >> (SectionSize-1))-1;
		if( ((HeightmapFileSize > 0 && (Width * Height * 2) == HeightmapFileSize) || HeightmapFileSize == INDEX_NONE ) &&
			Width > 0 && Height > 0 && SectionSize > 0 && NumSections > 0 &&		
			((Width-1) % (SectionSizeQuads * NumSections)) == 0 &&
			((Height-1) % (SectionSizeQuads * NumSections)) == 0)
		{
			ImportButton->IsEnabled = TRUE;
		}
		else
		{
			ImportButton->IsEnabled = FALSE;
		}

		if( String::Compare(Args->PropertyName, "CurrentLandscapeIndex")==0 )
		{
			ALandscape* PrevLandscape = LandscapeEditSystem->CurrentToolTarget.Landscape;
			LandscapeEditSystem->CurrentToolTarget.Landscape = NULL;

			if( CurrentLandscapeIndex >= 0 )
			{
				FString SelectedLandscape = CLRTools::ToFString(Landscapes[CurrentLandscapeIndex]);		

				for (FActorIterator It; It; ++It)
				{
					ALandscape* Landscape = Cast<ALandscape>(*It);
					if( Landscape )
					{
						if( SelectedLandscape == Landscape->GetName() )
						{
							LandscapeEditSystem->CurrentToolTarget.Landscape = Landscape;
							break;
						}
					}
				}
			}

			if( LandscapeEditSystem->CurrentToolTarget.Landscape != PrevLandscape )
			{
				UpdateTargets();
			}
		}

		if( String::Compare(Args->PropertyName, "CurrentTargetIndex")==0 )
		{
			// Heightmap
			if( CurrentTargetIndex < 1 )
			{
				LandscapeEditSystem->CurrentToolTarget.TargetType = LET_Heightmap;
			}
			else
			{
				LandscapeEditSystem->CurrentToolTarget.TargetType = LET_Weightmap;
				LandscapeEditSystem->CurrentToolTarget.LayerName = FName(*CLRTools::ToFString(LandscapeTargetsProperty[CurrentTargetIndex]->LayerName));
			}
		}

		if( String::Compare(Args->PropertyName, "IsViewModeDebugLayer")==0 )
		{
			// Heightmap
			if (GLandscapeViewMode == ELandscapeViewMode::DebugLayer && LandscapeEditSystem->CurrentToolTarget.Landscape)
			{
				LandscapeEditSystem->CurrentToolTarget.Landscape->UpdateDebugColorMaterial();
			}
		}
	}

	void OnLandscapeTargetLayerPropertyChanged( Object^ Owner, ComponentModel::PropertyChangedEventArgs^ Args )
	{
		if( String::Compare(Args->PropertyName, "Hardness")==0 )
		{
			LandscapeTarget^ Layer = safe_cast<LandscapeTarget^>(Owner);
			if (Layer->Index >= 0)
			{
				Layer->Hardness = Clamp<FLOAT>(Layer->Hardness, 0.f, 1.f);
				if (LandscapeEditSystem->CurrentToolTarget.Landscape)
				{
					LandscapeEditSystem->CurrentToolTarget.Landscape->LayerInfos(Layer->Index).Hardness = Layer->Hardness;
					LandscapeEditSystem->CurrentToolTarget.Landscape->PostEditChange();
				}
			}
			else
			{
				Layer->Hardness = -1;
			}
		}
		else if ( String::Compare(Args->PropertyName, "Viewmode")==0 )
		{
			LandscapeTarget^ Layer = safe_cast<LandscapeTarget^>(Owner);
			if (Layer->Index >= 0)
			{
				if (LandscapeEditSystem->CurrentToolTarget.Landscape)
				{
					INT ColorChannel = 0;
					if (Layer->ViewmodeR)
					{
						ColorChannel += 1;
					}
					if (Layer->ViewmodeG)
					{
						ColorChannel += 2;
					}
					if (Layer->ViewmodeB)
					{
						ColorChannel += 4;
					}

					LandscapeEditSystem->CurrentToolTarget.Landscape->LayerInfos(Layer->Index).DebugColorChannel = ColorChannel;
					LandscapeEditSystem->CurrentToolTarget.Landscape->UpdateDebugColorMaterial();
					UpdateTarget(Layer->Index);
					//UpdateTargets();
				}
				//appMsgf(AMT_OK, TEXT("%d %d %d"), (INT)Layer->ViewmodeR, (INT)Layer->ViewmodeG, (INT)Layer->ViewmodeB); // for test...
			}			
		}
	}

	void OnLandscapeTargetLayerChanged( Object^ Owner, ExecutedRoutedEventArgs^ Args )
	{
		TargetListBox->Focus();
	}

	/** Called in response to the user clicking the File/Open button */
	void HeightmapFileButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		// Prompt the user for the filenames
		WxFileDialog OpenFileDialog(GApp->EditorFrame, 
			*LocalizeUnrealEd("Import"),
			*CLRTools::ToFString(LastImportPath),
			TEXT(""),
			TEXT("Heightmap files (*.raw,*.r16)|*.raw;*.r16|All files (*.*)|*.*"),
			wxOPEN | wxFILE_MUST_EXIST,
			wxDefaultPosition
			);

		if( OpenFileDialog.ShowModal() == wxID_OK )
		{
			wxArrayString OpenFilePaths;
			OpenFileDialog.GetPaths(OpenFilePaths);	
			FFilename OpenFilename((const TCHAR*)OpenFilePaths[0]);
			HeightmapFileNameString = CLRTools::ToString(OpenFilename);
			LastImportPath = CLRTools::ToString(*OpenFilename.GetPath());
		}
	}

	void LayerFileButton_Click( Object^ Owner, ExecutedRoutedEventArgs^ Args )
	{
		LandscapeImportLayer^ Layer = safe_cast<LandscapeImportLayer^>(Args->Parameter);

		// Prompt the user for the filenames
		WxFileDialog OpenFileDialog(GApp->EditorFrame, 
			*LocalizeUnrealEd("Import"),
			*CLRTools::ToFString(LastImportPath),
			TEXT(""),
			TEXT("Layer files (*.raw,*.r8)|*.raw;*.r8|All files (*.*)|*.*"),
			wxOPEN | wxFILE_MUST_EXIST,
			wxDefaultPosition
			);

		if( OpenFileDialog.ShowModal() == wxID_OK )
		{
			wxArrayString OpenFilePaths;
			OpenFileDialog.GetPaths(OpenFilePaths);	
			FFilename OpenFilename((const TCHAR*)OpenFilePaths[0]);
			Layer->LayerFilename = CLRTools::ToString(OpenFilename);
			Layer->LayerName = CLRTools::ToString(*OpenFilename.GetBaseFilename());
			LastImportPath = CLRTools::ToString(*OpenFilename.GetPath());
			LandscapeImportLayersProperty->CheckNeedNewEntry();
		}
	}

	void LayerRemoveButton_Click( Object^ Owner, ExecutedRoutedEventArgs^ Args )
	{
		LandscapeImportLayer^ Layer = safe_cast<LandscapeImportLayer^>(Args->Parameter);
		LandscapeImportLayersProperty->Remove(Layer);
		LandscapeImportLayersProperty->CheckNeedNewEntry();
	}

	void EditHardness_Click( Object^ Owner, ExecutedRoutedEventArgs^ Args )
	{
		LandscapeTarget^ Layer = safe_cast<LandscapeTarget^>(Args->Parameter);
		Layer->Hardness = Clamp<FLOAT>(Layer->Hardness, 0.f, 1.f);
		ALandscape* Landscape = LandscapeEditSystem->CurrentToolTarget.Landscape;
		if (Landscape && CurrentTargetIndex-1 >= 0)
		{
			Landscape->LayerInfos(CurrentTargetIndex-1).Hardness = Layer->Hardness;
		}
	}

	void TargetLayerRemoveButton_Click( Object^ Owner, ExecutedRoutedEventArgs^ Args )
	{
		LandscapeTarget^ Layer = safe_cast<LandscapeTarget^>(Args->Parameter);

		if( appMsgf( AMT_YesNo, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("LandscapeMode_RemoveLayer"), *CLRTools::ToFString(Layer->LayerName))) ) )
		{
			ALandscape* Landscape = LandscapeEditSystem->CurrentToolTarget.Landscape;
			if( Landscape )
			{
				Landscape->DeleteLayer(FName(*CLRTools::ToFString(Layer->LayerName)));
			}
		}

		UpdateTargets();	
	}

	void AddLayerButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		Hardness = Clamp<FLOAT>(Hardness, 0.f, 1.f);
		ALandscape* Landscape = LandscapeEditSystem->CurrentToolTarget.Landscape;
		if( Landscape )
		{
			FName LayerFName = CLRTools::ToFName(AddLayerNameString);
			if( LayerFName == NAME_None )
			{
				appMsgf( AMT_OK, *LocalizeUnrealEd("LandscapeMode_AddLayerEnterName") );
			}
			// check for duplicate
			for (INT Idx = 0; Idx < Landscape->LayerInfos.Num(); Idx++)
			{
				if( Landscape->LayerInfos(Idx).LayerName == LayerFName )
				{
					appMsgf( AMT_OK, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("LandscapeMode_AddLayerDuplicate"), *LayerFName.ToString())) );
					return;
				}
			}
			new(Landscape->LayerInfos) FLandscapeLayerInfo(LayerFName, Hardness, bNoBlending);
		}
		AddLayerNameString = "";
		UpdateTargets();
	}

	/** Called when the Import button is clicked */
	void ImportButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		INT SectionSizeQuads = (256 >> (SectionSize-1))-1;
		TArray<BYTE> Data;
		if( HeightmapFileSize > 0 )
		{
			appLoadFileToArray(Data, *CLRTools::ToFString(HeightmapFileNameString));
		}
		else
		{
			// Initialize blank heightmap data
			Data.Add(Width*Height*2);
			WORD* WordData = (WORD*)&Data(0);
			for( INT i=0;i<Width*Height;i++ )
			{
				WordData[i] = 32768;
			}						
		}
		TArray<FLandscapeLayerInfo> LayerInfos;
		TArray<TArray<BYTE> > LayerDataArrays;
		TArray<BYTE*> LayerDataPtrs;

		for( INT LayerIndex=0;LayerIndex<LandscapeImportLayersProperty->Count;LayerIndex++ )
		{
			LandscapeImportLayer^ Layer = LandscapeImportLayersProperty[LayerIndex];

			FString LayerFilename = CLRTools::ToFString(Layer->LayerFilename);
			FString LayerName = CLRTools::ToFString(Layer->LayerName).Replace(TEXT(" "),TEXT(""));
			FLOAT Hardness = Clamp<FLOAT>(Layer->Hardness, 0.f, 1.f);
			BOOL NoBlending = Layer->NoBlending;
			
			if( LayerFilename != TEXT("") )
			{
				TArray<BYTE>* LayerData = new(LayerDataArrays)(TArray<BYTE>);
				appLoadFileToArray(*LayerData, *LayerFilename);

				if( LayerData->Num() != (Width*Height) )
				{
					appMsgf( AMT_OK, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("LandscapeImport_BadLayerSize"), *LayerFilename)) );
					return;
				}
				if( LayerName==TEXT("") )
				{
					appMsgf( AMT_OK, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("LandscapeImport_BadLayerName"), *LayerFilename)) );
					return;
				}

				new(LayerInfos) FLandscapeLayerInfo(FName(*LayerName), Hardness, NoBlending);
				LayerDataPtrs.AddItem(&(*LayerData)(0));
			}
			else
			if( LayerName!=TEXT("") )
			{
				// Add blank layer
				TArray<BYTE>* LayerData = new(LayerDataArrays)(TArray<BYTE>);
				LayerData->AddZeroed(Width*Height);
				new(LayerInfos) FLandscapeLayerInfo(FName(*LayerName), Hardness, NoBlending);
				LayerDataPtrs.AddItem(&(*LayerData)(0));
			}
		}

		ALandscape* Landscape = Cast<ALandscape>( GWorld->SpawnActor( ALandscape::StaticClass(), NAME_None, FVector(-64*Width,-64*Height,0) ) );
		Landscape->Import( Width,Height,SectionSizeQuads*NumSections,NumSections,SectionSizeQuads,(WORD*)&Data(0),LayerInfos,LayerDataPtrs.Num() ? &LayerDataPtrs(0) : NULL);

		// Clear out parameters
		HeightmapFileNameString = "";
		Width = 0;
		Height = 0;
		SectionSize = 0;
		NumSections = 0;
		TotalComponents = 0;
		LandscapeImportLayersProperty->Clear();
		LandscapeImportLayersProperty->CheckNeedNewEntry();
		UpdateLandscapeList();
	}

	void ExportButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		ALandscape* Landscape = LandscapeEditSystem->CurrentToolTarget.Landscape;
		if( Landscape )
		{
			TArray<FString> Filenames;

			for( INT i=-1;i<(bExportLayers ? Landscape->LayerInfos.Num() : 0);i++ )
			{
				FString SaveDialogTitle;
				FString DefaultFileName;
				FString FileTypes;

				if( i<0 )
				{
					SaveDialogTitle = *LocalizeUnrealEd("LandscapeExport_HeightmapFilename");
					DefaultFileName = TEXT("Heightmap.raw");
					FileTypes = TEXT("Heightmap .raw files|*.raw|Heightmap .r16 files|*.r16|All files|*.*");
				}
				else
				{
					SaveDialogTitle = *FString::Printf(LocalizeSecure(LocalizeUnrealEd("LandscapeExport_LayerFilename"), *Landscape->LayerInfos(i).LayerName.ToString()));
					DefaultFileName = *FString::Printf(TEXT("%s.raw"), *Landscape->LayerInfos(i).LayerName.ToString());
					FileTypes = TEXT("Layer .raw files|*.raw|Layer .r8 files|*.r8|All files|*.*");
				}

				// Prompt the user for the filenames
				WxFileDialog SaveFileDialog(GApp->EditorFrame, 
					*SaveDialogTitle,
					*CLRTools::ToFString(LastImportPath),
					*DefaultFileName,
					*FileTypes,
					wxSAVE | wxOVERWRITE_PROMPT,
					wxDefaultPosition
					);

				if( SaveFileDialog.ShowModal() != wxID_OK )
				{
					return;
				}

				wxArrayString SaveFilePaths;
				SaveFileDialog.GetPaths(SaveFilePaths);	
				FFilename SaveFilename((const TCHAR*)SaveFilePaths[0]);
				Filenames.AddItem(SaveFilename);
				LastImportPath = CLRTools::ToString(*SaveFilename.GetPath());
			}

			Landscape->Export(Filenames);
		}
	}

	/** Called when the Convert terrain button is clicked */
	void ConvertTerrainButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		// Temporary
		for (FActorIterator It; It; ++It)
		{
			ATerrain* OldTerrain = Cast<ATerrain>(*It);
			if( OldTerrain )
			{
				ALandscape* Landscape = Cast<ALandscape>( GWorld->SpawnActor( ALandscape::StaticClass(), NAME_None, OldTerrain->Location ) );
				if( Landscape->ImportFromOldTerrain(OldTerrain) )
				{
					GWorld->DestroyActor(OldTerrain);
				}
				else
				{
					GWorld->DestroyActor(Landscape);
				}
			}
		}
		UpdateLandscapeList();
	}

	void ToolButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		Visual^ RootVisual = InteropWindow->RootVisual;
		UniformGrid^ ToolGrid = safe_cast< UniformGrid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ToolGrid" ) );

		INT ButtonIndex = ToolGrid->Children->IndexOf(static_cast<ToggleButton^>(Owner));
		StackPanel^ ErosionTool = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ErosionPanel" ) );
		StackPanel^ NoiseTool = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "NoisePanel" ) );
		StackPanel^ HydraErosionTool = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "HydraulicErosionPanel" ) );
		StackPanel^ SmoothOptionPanel = safe_cast< StackPanel^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "SmoothOptionPanel" ) );

		if( ButtonIndex != -1 )
		{
			LandscapeEditSystem->SetCurrentTool(ButtonIndex);
			INT UIType = LandscapeEditSystem->ToolUIType;

			ErosionTool->Visibility = Visibility::Collapsed;
			NoiseTool->Visibility = Visibility::Collapsed;
			HydraErosionTool->Visibility = Visibility::Collapsed;
			SmoothOptionPanel->Visibility = Visibility::Collapsed;

			switch (UIType)
			{
			case ELandscapeToolUIType::Erosion:
				ErosionTool->Visibility = Visibility::Visible;
				break;
			case ELandscapeToolUIType::HydraErosion:
				HydraErosionTool->Visibility = Visibility::Visible;
				break;
			case ELandscapeToolUIType::Smooth:
				SmoothOptionPanel->Visibility = Visibility::Visible;
				break;
			case ELandscapeToolUIType::Noise:
				NoiseTool->Visibility = Visibility::Visible;
				break;
			case ELandscapeToolUIType::Default:
			default:
				break;
			}
		}
	}

	void BrushButton_SelectionChanged( Object^ Owner, RoutedEventArgs^ Args )
	{
		Visual^ RootVisual = InteropWindow->RootVisual;
		UniformGrid^ BrushGrid = safe_cast< UniformGrid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushGrid" ) );

		CustomControls::ToolDropdownRadioButton^ Button = static_cast<CustomControls::ToolDropdownRadioButton^>(Owner);
		INT ButtonIndex = BrushGrid->Children->IndexOf(Button);
		if( ButtonIndex != -1 )
		{
			FLandscapeBrushSet& BrushSet = LandscapeEditSystem->LandscapeBrushSets(ButtonIndex);

			if( BrushSet.Brushes.IsValidIndex(Button->SelectedIndex) )
			{
				FLandscapeBrush* NewBrush = BrushSet.Brushes(Button->SelectedIndex);
				if( LandscapeEditSystem->CurrentBrush != NewBrush )
				{
					LandscapeEditSystem->CurrentBrush->LeaveBrush();
					LandscapeEditSystem->CurrentBrush = NewBrush;
					LandscapeEditSystem->CurrentBrush->EnterBrush();
				}
			}
		}
	}

public:

	/** INotifyPropertyChanged: Exposed event from INotifyPropertyChanged::PropertyChanged */
	virtual event ComponentModel::PropertyChangedEventHandler^ PropertyChanged;


	/** Refresh all properties */
	void RefreshAllProperties()
	{
		// Pass null here which tells WPF that any or all properties may have changed
		OnPropertyChanged( nullptr );
	}

	Imaging::BitmapSource^ CreateBitmapSourceForThumbnail( const FObjectThumbnail& InThumbnail )
	{
		// Grab the raw thumbnail image data.  Note that this will decompress the image on demand if needed.
		const TArray< BYTE >& ThumbnailImageData = InThumbnail.GetUncompressedImageData();

		// Setup thumbnail metrics
		PixelFormat ImagePixelFormat = PixelFormats::Bgr32;
		const UINT ImageStride = InThumbnail.GetImageWidth() * sizeof( FColor );

		Imaging::BitmapSource^ MyBitmapSource = nullptr;
		if( ThumbnailImageData.Num() > 0 )
		{
			const UINT TotalImageBytes = ThumbnailImageData.Num();

			// @todo: Dots per inch.  Does it matter what we set this to?
			const int DPI = 96;

			System::IntPtr ImageDataPtr( ( PTRINT )&ThumbnailImageData( 0 ) );

			// Create the WPF bitmap object from our source data!
			MyBitmapSource = Imaging::BitmapSource::Create(
				InThumbnail.GetImageWidth(),		// Width
				InThumbnail.GetImageHeight(),		// Height
				DPI,								// Horizontal DPI
				DPI,								// Vertical DPI
				ImagePixelFormat,					// Pixel format
				nullptr,							// Palette
				ImageDataPtr,						// Image data
				TotalImageBytes,					// Size of image data
				ImageStride );						// Stride
		}

		return MyBitmapSource;
	}


	void UpdateLandscapeList()
	{
		FString CurrentLandscapeName = CLRTools::ToFString(LandscapeComboBox->Text);
		INT CurrentIndex = -1;

		ALandscape* PrevLandscape = LandscapeEditSystem->CurrentToolTarget.Landscape;
		LandscapeEditSystem->CurrentToolTarget.Landscape = NULL;
		Landscapes = gcnew List<String^>;

		INT Index = 0;
		for (FActorIterator It; It; ++It)
		{
			ALandscape* Landscape = Cast<ALandscape>(*It);
			if( Landscape )
			{
				if( CurrentLandscapeName == Landscape->GetName() || CurrentIndex == -1 )
				{
					CurrentIndex = Index;
					LandscapeEditSystem->CurrentToolTarget.Landscape = Landscape;
				}

				Landscapes->Add(CLRTools::ToString(*Landscape->GetName()));
				Index++;
			}
		}

		LandscapeComboBox->SelectedIndex = CurrentIndex;

		if( LandscapeEditSystem->CurrentToolTarget.Landscape != PrevLandscape )
		{
			UpdateTargets();
		}
	}

	void UpdateTargets()
	{
		TArray<FLandscapeLayerThumbnailInfo> LayerThumbnailInfo;
		LandscapeEditSystem->GetLayersAndThumbnails(LayerThumbnailInfo);

		LandscapeTargetsProperty->Clear();

		if( LandscapeEditSystem->CurrentToolTarget.Landscape )
		{
			LandscapeTargetsProperty->Add( gcnew LandscapeTarget("Height map", "", gcnew BitmapImage( gcnew Uri( gcnew System::String(*FString::Printf(TEXT("%swxres\\Landscape_Target_Heightmap.png"), *GetEditorResourcesDir())), UriKind::Absolute) ), -1.f, FALSE, -1, -1) );
		}

		for( INT i=0;i<LayerThumbnailInfo.Num();i++ )
		{
			LandscapeTarget^ L = gcnew LandscapeTarget(CLRTools::ToString(LayerThumbnailInfo(i).LayerInfo.LayerName.ToString()), "", CreateBitmapSourceForThumbnail(LayerThumbnailInfo(i).LayerThumbnail), LayerThumbnailInfo(i).LayerInfo.Hardness, LayerThumbnailInfo(i).LayerInfo.bNoWeightBlend, LayerThumbnailInfo(i).LayerInfo.DebugColorChannel, i);
			L->PropertyChanged += gcnew ComponentModel::PropertyChangedEventHandler( this, &MLandscapeEditWindow::OnLandscapeTargetLayerPropertyChanged );
			LandscapeTargetsProperty->Add( L );
		}
	}

	void UpdateTarget(int Index)
	{
		if (Index >= 0 && Index+1 < LandscapeTargetsProperty->Count && LandscapeEditSystem->CurrentToolTarget.Landscape) // Update only one index
		{
			LandscapeTarget^ Old = LandscapeTargetsProperty[Index+1];
			LandscapeTarget^ L = gcnew LandscapeTarget(Old->LayerName, Old->LayerDescription, Old->Image, LandscapeEditSystem->CurrentToolTarget.Landscape->LayerInfos(Index).Hardness, 
				LandscapeEditSystem->CurrentToolTarget.Landscape->LayerInfos(Index).bNoWeightBlend, LandscapeEditSystem->CurrentToolTarget.Landscape->LayerInfos(Index).DebugColorChannel, Index);
			L->PropertyChanged += gcnew ComponentModel::PropertyChangedEventHandler( this, &MLandscapeEditWindow::OnLandscapeTargetLayerPropertyChanged );
			LandscapeTargetsProperty->RemoveAt(Index+1);
			LandscapeTargetsProperty->Insert(Index+1, L);
		}
	}


	void NotifyCurrentToolChanged( INT ToolIndex )
	{
		Visual^ RootVisual = InteropWindow->RootVisual;
		UniformGrid^ ToolGrid = safe_cast< UniformGrid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ToolGrid" ) );

		for( INT ButtonIndex=0;ButtonIndex<ToolGrid->Children->Count;ButtonIndex++ )
		{
			ToggleButton^ Btn = static_cast<ToggleButton^>(ToolGrid->Children[ButtonIndex]);
			Btn->IsChecked = (ButtonIndex == ToolIndex);
		}
	}


	/** Called when a property has changed */
	virtual void OnPropertyChanged( String^ Info )
	{
		PropertyChanged( this, gcnew ComponentModel::PropertyChangedEventArgs( Info ) );
	}

	/** Copy the window position back to the edit mode before the window closes */
	void SaveWindowSettings()
	{
		RECT rect;
		GetWindowRect( GetWindowHandle(), &rect );	
		LandscapeEditSystem->UISettings.SetWindowSizePos(rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top);
	}

protected:

	/** Pointer to the landscape edit system that owns us */
	FEdModeLandscape* LandscapeEditSystem;

	/** Windows message hook - handle resize border */
	virtual IntPtr VirtualMessageHookFunction( IntPtr HWnd, int Msg, IntPtr WParam, IntPtr LParam, bool% OutHandled ) override
	{
		const INT FakeTopBottomResizeHeight = 5;

		OutHandled = false;
		int RetVal = 0;

		switch( Msg )
		{
		case WM_NCHITTEST:
			// Override the client area detection to allow resizing with the top and bottom border.
			{
				if( FakeTopBottomResizeHeight > 0 )
				{
					const HWND NativeHWnd = ( HWND )( PTRINT )HWnd;
					const LPARAM NativeWParam = ( WPARAM )( PTRINT )WParam;
					const LPARAM NativeLParam = ( WPARAM )( PTRINT )LParam;

					// Let Windows perform the true hit test
					RetVal = DefWindowProc( NativeHWnd, Msg, NativeWParam, NativeLParam );

					// Did the user click in the client area?
					if( RetVal == HTCLIENT )
					{
						// Grab the size of our window
						RECT WindowRect;
						::GetWindowRect( NativeHWnd, &WindowRect );

						int CursorXPos = GET_X_LPARAM( NativeLParam );
						int CursorYPos = GET_Y_LPARAM( NativeLParam );

						if( FakeTopBottomResizeHeight > 0 && CursorYPos >= WindowRect.top && CursorYPos < WindowRect.top + FakeTopBottomResizeHeight )
						{
							// Trick Windows into thinking the user interacted with top resizing border
							RetVal = HTTOP;
							OutHandled = true;
						}
						else
						if( FakeTopBottomResizeHeight > 0 && CursorYPos <= WindowRect.bottom && CursorYPos > WindowRect.bottom - FakeTopBottomResizeHeight )
						{
							// Trick Windows into thinking the user interacted with bottom resizing border
							RetVal = HTBOTTOM;
							OutHandled = true;
						}
					}
				}
			}
			break;
		case WM_GETMINMAXINFO:
			// Limit minimum vertical size to 150 pixels
			{
				const HWND NativeHWnd = ( HWND )( PTRINT )HWnd;
				const LPARAM NativeWParam = ( WPARAM )( PTRINT )WParam;
				const LPARAM NativeLParam = ( WPARAM )( PTRINT )LParam;
				RetVal = DefWindowProc( NativeHWnd, Msg, NativeWParam, NativeLParam );
				OutHandled = true;

				((MINMAXINFO*)(NativeLParam))->ptMinTrackSize.y = 150;
			}
			break;
		case WM_MOUSEACTIVATE:
			// Bring window to front when clicking on it.
			{
				const HWND NativeHWnd = ( HWND )( PTRINT )HWnd;
				BringWindowToTop(NativeHWnd);
			}
			break;
		}

		if( OutHandled )
		{
			return (IntPtr)RetVal;
		}

		return MWPFWindowWrapper::VirtualMessageHookFunction( HWnd, Msg, WParam, LParam, OutHandled );
	}
};



/** Static: Allocate and initialize landscape edit window */
FLandscapeEditWindow* FLandscapeEditWindow::CreateLandscapeEditWindow( FEdModeLandscape* InLandscapeEditSystem, const HWND InParentWindowHandle )
{
	FLandscapeEditWindow* NewLandscapeEditWindow = new FLandscapeEditWindow();

	if( !NewLandscapeEditWindow->InitLandscapeEditWindow( InLandscapeEditSystem, InParentWindowHandle ) )
	{
		delete NewLandscapeEditWindow;
		return NULL;
	}

	return NewLandscapeEditWindow;
}



/** Constructor */
FLandscapeEditWindow::FLandscapeEditWindow()
{
	// Register to find out about other windows going modal
	GCallbackEvent->Register( CALLBACK_EditorPreModal, this );
	GCallbackEvent->Register( CALLBACK_EditorPostModal, this );
	GCallbackEvent->Register( CALLBACK_MapChange, this );
	GCallbackEvent->Register( CALLBACK_WorldChange, this );
	GCallbackEvent->Register( CALLBACK_ObjectPropertyChanged, this );
}



/** Destructor */
FLandscapeEditWindow::~FLandscapeEditWindow()
{
	// Unregister callbacks
	GCallbackEvent->UnregisterAll( this );

	// @todo WPF: This is probably redundant, but I'm still not sure if AutoGCRoot destructor will get
	//   called when native code destroys an object that has a non-virtual (or no) destructor

	// Dispose of WindowControl
	WindowControl.reset();
}



/** Initialize the landscape edit window */
UBOOL FLandscapeEditWindow::InitLandscapeEditWindow( FEdModeLandscape* InLandscapeEditSystem, const HWND InParentWindowHandle )
{
	WindowControl = gcnew MLandscapeEditWindow(InLandscapeEditSystem);

	UBOOL bSuccess = WindowControl->InitLandscapeEditWindow( InParentWindowHandle );

	return bSuccess;
}



/** Refresh all properties */
void FLandscapeEditWindow::RefreshAllProperties()
{
	WindowControl->RefreshAllProperties();
}



/** Saves window settings to the Landscape Edit settings structure */
void FLandscapeEditWindow::SaveWindowSettings()
{
	WindowControl->SaveWindowSettings();
}



/** Returns true if the mouse cursor is over the landscape edit window */
UBOOL FLandscapeEditWindow::IsMouseOverWindow()
{
	if( WindowControl.get() != nullptr )
	{
		FrameworkElement^ WindowContentElement = safe_cast<FrameworkElement^>( WindowControl->GetRootVisual() );
		if( WindowContentElement->IsMouseOver )
		{
			return TRUE;
		}
	}

	return FALSE;
}

/** FCallbackEventDevice: Called when a parameterless global event we've registered for is fired */
void FLandscapeEditWindow::Send( ECallbackEventType Event )
{
	if( WindowControl.get() != nullptr )
	{
		FrameworkElement^ WindowContentElement = safe_cast<FrameworkElement^>( WindowControl->GetRootVisual() );

		switch ( Event )
		{
			case CALLBACK_EditorPreModal:
				WindowContentElement->IsEnabled = false;
				break;

			case CALLBACK_EditorPostModal:
				WindowContentElement->IsEnabled = true;
				break;

			case CALLBACK_MapChange:
			case CALLBACK_WorldChange:
			case CALLBACK_ObjectPropertyChanged:
				WindowControl->UpdateLandscapeList();
				break;
		}
	}
}

/* User changed the current tool - update the button state */
void FLandscapeEditWindow::NotifyCurrentToolChanged( INT ToolIndex )
{
	if( WindowControl.get() != nullptr )
	{
		return WindowControl->NotifyCurrentToolChanged(ToolIndex);
	}
}

