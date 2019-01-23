/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEdCLR.h"

#include "ManagedCodeSupportCLR.h"
#include "MeshPaintWindowShared.h"
#include "ColorPickerShared.h"
#include "WPFFrameCLR.h"

#pragma unmanaged
#include "MeshPaintEdMode.h"
#pragma managed

#include "ConvertersCLR.h"

/** Converts a Total Weight Count value to a Visibility state based on the Paint Weight Index specified
    in the converter's parameter */
[ValueConversion(int::typeid, Visibility::typeid)]
ref class TotalWeightCountToVisibilityConverter : public IValueConverter
{

public:
    
	virtual Object^ Convert( Object^ value, Type^ targetType, Object^ parameter, Globalization::CultureInfo^ culture )
    {
		int TotalWeightCount = (int)value;
		int PaintWeightIndex = (int)parameter;
		
		if( PaintWeightIndex < TotalWeightCount )
		{
			return Visibility::Visible;
		}

		return Visibility::Collapsed;
    }

    virtual Object^ ConvertBack( Object^ value, Type^ targetType, Object^ parameter, Globalization::CultureInfo^ culture )
    {
		// Not supported
		return nullptr;
    }
};



/**
 * Mesh Paint window control (managed)
 */
ref class MMeshPaintWindow
	: public MWPFPanel
{
public:
	/**
	 * Initialize the mesh paint window
	 *
	 * @param	InMeshPaintSystem		Mesh paint system that owns us
	 * @param	InParentWindowHandle	Parent window handle
	 *
	 * @return	TRUE if successful
	 */
	MMeshPaintWindow( MWPFFrame^ InFrame, FEdModeMeshPaint* InMeshPaintSystem, String^ XamlFileName, FPickColorStruct& PaintColorStruct, FPickColorStruct& EraseColorStruct )
		: MWPFPanel(XamlFileName)
	{
		check( InMeshPaintSystem != NULL );
		MeshPaintSystem = InMeshPaintSystem;

		// Setup bindings
		Visual^ RootVisual = this;

		FrameworkElement^ WindowContentElement = safe_cast<FrameworkElement^>(RootVisual);
		WindowContentElement->DataContext = this;

		// Setup auto-collapsing sub panel bindings
		UnrealEd::Utils::CreateBinding(
			safe_cast< Grid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "VertexPaintGrid" ) ),
			Grid::VisibilityProperty, this, "IsResourceTypeVertexColors", gcnew BooleanToVisibilityConverter() );	
		UnrealEd::Utils::CreateBinding(
			safe_cast< Grid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "TexturePaintGrid" ) ),
			Grid::VisibilityProperty, this, "IsResourceTypeTexture", gcnew BooleanToVisibilityConverter() );	


		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "VertexPaintTargetRadio_ComponentInstance" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsVertexPaintTargetComponentInstance" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "VertexPaintTargetRadio_Mesh" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsVertexPaintTargetMesh" );


		UnrealEd::Utils::CreateBinding(
			safe_cast< TextBlock^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "InstanceVertexColorsText" ) ),
			TextBlock::TextProperty, this, "InstanceVertexColorsText" );
		
		Button^ RemoveInstanceVertexColorsButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "RemoveInstanceVertexColorsButton" ) );
		UnrealEd::Utils::CreateBinding(
			RemoveInstanceVertexColorsButton,
			Button::IsEnabledProperty, this, "HasInstanceVertexColors" );
		RemoveInstanceVertexColorsButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::RemoveInstanceVertexColorsButton_Click );
		
		Button^ FixInstanceVertexColorsButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "FixupInstanceVertexColorsButton" ) );
		UnrealEd::Utils::CreateBinding(
			FixInstanceVertexColorsButton,
			Button::IsEnabledProperty, this, "RequiresInstanceVertexColorsFixup" );
		FixInstanceVertexColorsButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::FixInstanceVertexColorsButton_Click );

		Button^ FillInstanceVertexColorsButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "FillInstanceVertexColorsButton" ) );
		FillInstanceVertexColorsButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::FillInstanceVertexColorsButton_Click );

		Button^ PushInstanceVertexColorsToMeshButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PushInstanceVertexColorsToMeshButton" ) );
		PushInstanceVertexColorsToMeshButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::PushInstanceVertexColorsToMeshButton_Click );
		//hide push to mesh button when editing an assets colors
		UnrealEd::Utils::CreateBinding(
			safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PushInstanceVertexColorsToMeshButton" ) ),
			Button::VisibilityProperty, this, "IsVertexPaintTargetComponentInstance", gcnew BooleanToVisibilityConverter() );	


 		Button^ CreateMatTexButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "CreateInstanceMaterialAndTextureButton" ) );
		UnrealEd::Utils::CreateBinding(
			CreateMatTexButton,
			Button::IsEnabledProperty, this, "CanCreateInstanceMaterialAndTexture" );
 		CreateMatTexButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::CreateInstanceMaterialAndTextureButton_Click );

 		Button^ RemoveMatTexButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "RemoveInstanceMaterialAndTextureButton" ) );
		UnrealEd::Utils::CreateBinding(
			RemoveMatTexButton,
			Button::IsEnabledProperty, this, "HasInstanceMaterialAndTexture" );
 		RemoveMatTexButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::RemoveInstanceMaterialAndTextureButton_Click );


		UnrealEd::Utils::CreateBinding(
			safe_cast< ComboBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "UVChannelCombo" ) ),
			ComboBox::SelectedIndexProperty, this, "UVChannel", gcnew IntToIntOffsetConverter( 0 ) );


		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintModeRadio_Colors" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintModeColors" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintModeRadio_Weights" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintModeWeights" );	



		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushRadiusDragSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "BrushRadius" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushFalloffAmountDragSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "BrushFalloffAmount" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "BrushStrengthDragSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "BrushStrength" );
 		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EnableFlowCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "EnableFlow" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< CustomControls::DragSlider^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "FlowAmountDragSlider" ) ),
			CustomControls::DragSlider::ValueProperty, this, "FlowAmount" );
 		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "IgnoreBackFacingCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "IgnoreBackFacing" );


 		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "WriteRedCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "WriteRed" );
 		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "WriteGreenCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "WriteGreen" );
 		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "WriteBlueCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "WriteBlue" );
 		UnrealEd::Utils::CreateBinding(
			safe_cast< CheckBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "WriteAlphaCheckBox" ) ),
			CheckBox::IsCheckedProperty, this, "WriteAlpha" );



		// Setup auto-collapsing sub panel bindings
		// Xaml: Visibility="{Binding Path=IsPaintModeColors, Converter={StaticResource BoolToVisConverter}}"
		UnrealEd::Utils::CreateBinding(
			safe_cast< Grid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintColorsGrid" ) ),
			Grid::VisibilityProperty, this, "IsPaintModeColors", gcnew BooleanToVisibilityConverter() );	
		UnrealEd::Utils::CreateBinding(
			safe_cast< Grid^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightsGrid" ) ),
			Grid::VisibilityProperty, this, "IsPaintModeWeights", gcnew BooleanToVisibilityConverter() );	


// 		if( 0 )
// 		{
// 			// NOTE: WPF currently has a bug where RadioButtons won't propagate "unchecked" state to bound properties.
// 			//		 We work around this by using an event handler to make sure our properties are in sync.
// 			RadioButton^ Radio = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintModeRadio_Colors" ) );
// 			Radio->Checked += gcnew RoutedEventHandler( this, &MMeshPaintWindow::OnPaintModeRadioButtonChecked );
// 			Radio->Unchecked += gcnew RoutedEventHandler( this, &MMeshPaintWindow::OnPaintModeRadioButtonChecked );
// 			Radio->IsChecked = this->IsPaintModeColors;
// 		}


 		UnrealEd::Utils::CreateBinding(
			safe_cast< ComboBox^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "TotalWeightCountCombo" ) ),
			ComboBox::SelectedIndexProperty, this, "TotalWeightCount", gcnew IntToIntOffsetConverter( -2 ) );


		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_1" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintWeightIndex1" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_2" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintWeightIndex2" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_3" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintWeightIndex3" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_4" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintWeightIndex4" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_5" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsPaintWeightIndex5" );

		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_1" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsEraseWeightIndex1" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_2" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsEraseWeightIndex2" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_3" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsEraseWeightIndex3" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_4" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsEraseWeightIndex4" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_5" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsEraseWeightIndex5" );

		// Auto-collapse Paint Weight Index radio buttons based on the Total Weight Count
		UnrealEd::Utils::CreateBinding(
			safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_3" ) ),
			RadioButton::VisibilityProperty, this, "TotalWeightCount", gcnew TotalWeightCountToVisibilityConverter(), 2 );	
		UnrealEd::Utils::CreateBinding(
			safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_4" ) ),
			RadioButton::VisibilityProperty, this, "TotalWeightCount", gcnew TotalWeightCountToVisibilityConverter(), 3 );
		UnrealEd::Utils::CreateBinding(
			safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_5" ) ),
			RadioButton::VisibilityProperty, this, "TotalWeightCount", gcnew TotalWeightCountToVisibilityConverter(), 4 );

		// Auto-collapse Erase Weight Index radio buttons based on the Total Weight Count
		UnrealEd::Utils::CreateBinding(
			safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_3" ) ),
			RadioButton::VisibilityProperty, this, "TotalWeightCount", gcnew TotalWeightCountToVisibilityConverter(), 2 );	
		UnrealEd::Utils::CreateBinding(
			safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_4" ) ),
			RadioButton::VisibilityProperty, this, "TotalWeightCount", gcnew TotalWeightCountToVisibilityConverter(), 3 );
		UnrealEd::Utils::CreateBinding(
			safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseWeightIndexRadio_5" ) ),
			RadioButton::VisibilityProperty, this, "TotalWeightCount", gcnew TotalWeightCountToVisibilityConverter(), 4 );


		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ResourceTypeRadio_VertexColors" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsResourceTypeVertexColors" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ResourceTypeRadio_Texture" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsResourceTypeTexture" );


		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Normal" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsColorViewModeNormal" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_RGB" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsColorViewModeRGB" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Alpha" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsColorViewModeAlpha" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Red" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsColorViewModeRed" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Green" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsColorViewModeGreen" );
		UnrealEd::Utils::CreateBinding(
			safe_cast< UnrealEd::BindableRadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Blue" ) ),
			UnrealEd::BindableRadioButton::IsActuallyCheckedProperty, this, "IsColorViewModeBlue" );


		// Register for property change callbacks from our properties object
		this->PropertyChanged += gcnew ComponentModel::PropertyChangedEventHandler( this, &MMeshPaintWindow::OnMeshPaintPropertyChanged );


		Button^ SwapPaintAndEraseColorButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "SwapPaintAndEraseColorButton" ) );
		SwapPaintAndEraseColorButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::SwapPaintColorButton_Click );

		Button^ SwapPaintAndEraseWeightIndexButton = safe_cast< Button^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "SwapPaintAndEraseWeightIndexButton" ) );
		SwapPaintAndEraseWeightIndexButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::SwapPaintWeightIndexButton_Click );

		//which paint color to edit
		RadioButton^ PaintColorButton = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintColorButton" ) );
		PaintColorButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::EditPaintColor);
		RadioButton^ EraseColorButton = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "EraseColorButton" ) );
		EraseColorButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::EditEraseColor);

		//tell the color picker panels that they don't need ok/cancel buttons or previous color previews
		const UBOOL bIntegratedPanel = TRUE;

		//make paint sub panel
		PaintColorPanel = gcnew MColorPickerPanel(PaintColorStruct, CLRTools::ToString(TEXT("ColorPickerWindow.xaml")), bIntegratedPanel);
		PaintColorStruct.FLOATColorArray.AddItem(&(FMeshPaintSettings::Get().PaintColor));
		PaintColorPanel->BindData();
		PaintColorPanel->SetParentFrame(InFrame);
		//make erase sub panel
		EraseColorPanel = gcnew MColorPickerPanel(EraseColorStruct, CLRTools::ToString(TEXT("ColorPickerWindow.xaml")), bIntegratedPanel);
		EraseColorStruct.FLOATColorArray.AddItem(&(FMeshPaintSettings::Get().EraseColor));
		EraseColorPanel->BindData();
		EraseColorPanel->SetParentFrame(InFrame);

		//bind preview color update to color picker callback
		PaintColorPanel->AddCallbackDelegate(gcnew ColorPickerPropertyChangeFunction(this, &MMeshPaintWindow::UpdatePreviewColors));
		EraseColorPanel->AddCallbackDelegate(gcnew ColorPickerPropertyChangeFunction(this, &MMeshPaintWindow::UpdatePreviewColors));

		//make erase sub panel
		UpdatePreviewColors();

		//embed color pickers
		Border^ PaintColorBorder = safe_cast< Border^ >( LogicalTreeHelper::FindLogicalNode( this, "PaintColorBorder" ) );
		PaintColorBorder->Child = PaintColorPanel;
		Border^ EraseColorBorder = safe_cast< Border^ >( LogicalTreeHelper::FindLogicalNode( this, "EraseColorBorder" ) );
		EraseColorBorder->Child = EraseColorPanel;
		EraseColorPanel->Visibility = System::Windows::Visibility::Collapsed;
	}

	virtual ~MMeshPaintWindow(void)
	{
		delete PaintColorPanel;
		delete EraseColorPanel;
	}

	virtual void SetParentFrame( MWPFFrame^ InParentFrame ) override
	{
		MWPFPanel::SetParentFrame( InParentFrame );

		// Setup a handler for when the closed button is pressed
		Button^ CloseButton = safe_cast<Button^>( LogicalTreeHelper::FindLogicalNode( InParentFrame->GetRootVisual(), "TitleBarCloseButton" ) );
		CloseButton->Click += gcnew RoutedEventHandler( this, &MMeshPaintWindow::OnClose );
	}

	/** Called when the close button on the title bar is pressed */
	void OnClose( Object^ Sender, RoutedEventArgs^ Args )
	{
		// Deactivate mesh paint when the close button is pressed
		GEditorModeTools().DeactivateMode( EM_MeshPaint );
	}

protected:

	/** Called when a mesh paint window property is changed */
	void OnMeshPaintPropertyChanged( Object^ Owner, ComponentModel::PropertyChangedEventArgs^ Args )
	{
		// ...

		if( Args->PropertyName == nullptr || Args->PropertyName->StartsWith( "TotalWeightCount" ) )
		{
			// Make sure that a valid paint weight index is selected after the total weight count has changed
			if( IsPaintWeightIndex3 && TotalWeightCount < 3 )
			{
				IsPaintWeightIndex2 = true;
			}
			else if( IsPaintWeightIndex4 && TotalWeightCount < 4 )
			{
				IsPaintWeightIndex3 = true;
			}
			else if( IsPaintWeightIndex5 && TotalWeightCount < 5 )
			{
				IsPaintWeightIndex4 = true;
			}
		}
	}

	/** Called when the swap paint color button is clicked */
	void SwapPaintColorButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		FLinearColor TempColor = FMeshPaintSettings::Get().PaintColor;
		FMeshPaintSettings::Get().PaintColor = FMeshPaintSettings::Get().EraseColor;
		FMeshPaintSettings::Get().EraseColor = TempColor;

		PaintColorPanel->BindData();
		EraseColorPanel->BindData();
		UpdatePreviewColors();
	}

	/**
	 * Hides erase color picker and shows the paint color picker
	 */
	void EditPaintColor( Object^ Owner, RoutedEventArgs^ Args )
	{
		PaintColorPanel->Visibility = System::Windows::Visibility::Visible;
		EraseColorPanel->Visibility = System::Windows::Visibility::Collapsed;
	}

	/**
	 * Hides paint color picker and shows the erase color picker
	 */
	void EditEraseColor( Object^ Owner, RoutedEventArgs^ Args )
	{
		PaintColorPanel->Visibility = System::Windows::Visibility::Collapsed;
		EraseColorPanel->Visibility = System::Windows::Visibility::Visible;
	}

	/**
	* Updates the preview color (original color)
	 */
	void UpdatePreviewColors()
	{
		FLinearColor PaintColor = FMeshPaintSettings::Get().PaintColor;
		FLinearColor EraseColor = FMeshPaintSettings::Get().EraseColor;

		//PAINT
		System::Windows::Shapes::Rectangle^ PaintRect = safe_cast< System::Windows::Shapes::Rectangle^ >( LogicalTreeHelper::FindLogicalNode( this, "PaintColorRectNoAlpha" ) );
		PaintRect->Fill = gcnew SolidColorBrush( MColorPickerPanel::GetBrushColor(1.0f, PaintColor.R, PaintColor.G, PaintColor.B) );

		System::Windows::Shapes::Rectangle^ PaintRectNoAlpha = safe_cast< System::Windows::Shapes::Rectangle^ >( LogicalTreeHelper::FindLogicalNode( this, "PaintColorRect" ) );
		PaintRectNoAlpha->Fill = gcnew SolidColorBrush( MColorPickerPanel::GetBrushColor(PaintColor.A, PaintColor.R, PaintColor.G, PaintColor.B) );

		//ERASE
		System::Windows::Shapes::Rectangle^ EraseRect = safe_cast< System::Windows::Shapes::Rectangle^ >( LogicalTreeHelper::FindLogicalNode( this, "EraseColorRectNoAlpha" ) );
		EraseRect->Fill = gcnew SolidColorBrush( MColorPickerPanel::GetBrushColor(1.0f, EraseColor.R, EraseColor.G, EraseColor.B) );

		System::Windows::Shapes::Rectangle^ EraseRectNoAlpha = safe_cast< System::Windows::Shapes::Rectangle^ >( LogicalTreeHelper::FindLogicalNode( this, "EraseColorRect" ) );
		EraseRectNoAlpha->Fill = gcnew SolidColorBrush( MColorPickerPanel::GetBrushColor(EraseColor.A, EraseColor.R, EraseColor.G, EraseColor.B) );
	}



	
	/** Called when the swap paint weight index button is clicked */
	void SwapPaintWeightIndexButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		int TempWeightIndex = FMeshPaintSettings::Get().PaintWeightIndex;
		FMeshPaintSettings::Get().PaintWeightIndex = FMeshPaintSettings::Get().EraseWeightIndex;
		FMeshPaintSettings::Get().EraseWeightIndex = TempWeightIndex;

		// Call the property changed handler ourself since we circumvented the get/set accessors
		RefreshAllProperties();
	}


	
	/** Called when the remove instance vertex colors button is clicked */
	void RemoveInstanceVertexColorsButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		MeshPaintSystem->RemoveInstanceVertexColors();
	}

	/** Called when the fixup instance vertex colors button is clicked */
	void FixInstanceVertexColorsButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		MeshPaintSystem->FixupInstanceVertexColors();
	}

	/** Called to flood fill the vertex color with the current paint color */
	void FillInstanceVertexColorsButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		MeshPaintSystem->FillInstanceVertexColors();
	}

	/** Called to push instance vertex color to the mesh */
	void PushInstanceVertexColorsToMeshButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		MeshPaintSystem->PushInstanceVertexColorsToMesh();
	}

	/** Called when the create material/texture button is clicked */
	void CreateInstanceMaterialAndTextureButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		MeshPaintSystem->CreateInstanceMaterialAndTexture();
	}



	/** Called when the remove material/texture button is clicked */
	void RemoveInstanceMaterialAndTextureButton_Click( Object^ Owner, RoutedEventArgs^ Args )
	{
		MeshPaintSystem->RemoveInstanceMaterialAndTexture();
	}

// 
// 	/** Called when any of the paint mode radio buttons are clicked */
// 	void OnPaintModeRadioButtonChecked( Object^ Sender, RoutedEventArgs^ Args )
// 	{
// 		Visual^ RootVisual = InteropWindow->RootVisual;
// 
// 		RadioButton^ RadioColors = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintModeRadio_Colors" ) );
// 		RadioButton^ RadioWeights = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintModeRadio_Weights" ) );
// 
// 		IsPaintModeColors = RadioColors->IsChecked.Value;
// 		IsPaintModeWeights = RadioWeights->IsChecked.Value;
// 	}
// 
// 
// 	/** Called when any of the paint weight index radio buttons are clicked */
// 	void OnPaintWeightIndexRadioButtonChecked( Object^ Sender, RoutedEventArgs^ Args )
// 	{
// 		Visual^ RootVisual = InteropWindow->RootVisual;
// 
// 		RadioButton^ RadioWeight1 = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_1" ) );
// 		RadioButton^ RadioWeight2 = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_2" ) );
// 		RadioButton^ RadioWeight3 = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_3" ) );
// 		RadioButton^ RadioWeight4 = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_4" ) );
// 		RadioButton^ RadioWeight5 = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "PaintWeightIndexRadio_5" ) );
// 
// 		IsPaintWeightIndex1 = RadioWeight1->IsChecked.Value;
// 		IsPaintWeightIndex2 = RadioWeight2->IsChecked.Value;
// 		IsPaintWeightIndex3 = RadioWeight3->IsChecked.Value;
// 		IsPaintWeightIndex4 = RadioWeight4->IsChecked.Value;
// 		IsPaintWeightIndex5 = RadioWeight5->IsChecked.Value;
// 	}
// 
// 
// 	/** Called when any of the color view mode radio buttons are clicked */
// 	void OnColorViewModeRadioButtonChecked( Object^ Sender, RoutedEventArgs^ Args )
// 	{
// 		Visual^ RootVisual = InteropWindow->RootVisual;
// 
// 		RadioButton^ RadioNormal = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Normal" ) );
// 		RadioButton^ RadioRGB = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_RGB" ) );
// 		RadioButton^ RadioAlpha = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Alpha" ) );
// 		RadioButton^ RadioRed = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Red" ) );
// 		RadioButton^ RadioGreen = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Green" ) );
// 		RadioButton^ RadioBlue = safe_cast< RadioButton^ >( LogicalTreeHelper::FindLogicalNode( RootVisual, "ColorViewModeRadio_Blue" ) );
// 
// 		IsColorViewModeNormal = RadioNormal->IsChecked.Value;
// 		IsColorViewModeRGB = RadioRGB->IsChecked.Value;
// 		IsColorViewModeAlpha = RadioAlpha->IsChecked.Value;
// 		IsColorViewModeRed = RadioRed->IsChecked.Value;
// 		IsColorViewModeGreen = RadioGreen->IsChecked.Value;
// 		IsColorViewModeBlue = RadioBlue->IsChecked.Value;
// 	}
// 


public:

	/** BrushRadius property */
	DECLARE_MAPPED_NOTIFY_PROPERTY( double, BrushRadius, FLOAT, FMeshPaintSettings::Get().BrushRadius );

	/** BrushFalloffAmount property */
	DECLARE_MAPPED_NOTIFY_PROPERTY( double, BrushFalloffAmount, FLOAT, FMeshPaintSettings::Get().BrushFalloffAmount );

	/** BrushStrength property */
	DECLARE_MAPPED_NOTIFY_PROPERTY( double, BrushStrength, FLOAT, FMeshPaintSettings::Get().BrushStrength );

	/** EnableFlow property */
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( EnableFlow, FMeshPaintSettings::Get().bEnableFlow );

	/** FlowAmount property */
	DECLARE_MAPPED_NOTIFY_PROPERTY( double, FlowAmount, FLOAT, FMeshPaintSettings::Get().FlowAmount );

	/** IgnoreBackFacing property */
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( IgnoreBackFacing, FMeshPaintSettings::Get().bOnlyFrontFacingTriangles );


	/** Radio button properties for Resource Type */
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsResourceTypeVertexColors, EMeshPaintResource::Type, FMeshPaintSettings::Get().ResourceType, EMeshPaintResource::VertexColors );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsResourceTypeTexture, EMeshPaintResource::Type, FMeshPaintSettings::Get().ResourceType, EMeshPaintResource::Texture );


	/** Radio button properties for Vertex Paint Target */
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsVertexPaintTargetComponentInstance, EMeshVertexPaintTarget::Type, FMeshPaintSettings::Get().VertexPaintTarget, EMeshVertexPaintTarget::ComponentInstance );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsVertexPaintTargetMesh, EMeshVertexPaintTarget::Type, FMeshPaintSettings::Get().VertexPaintTarget, EMeshVertexPaintTarget::Mesh );


	/** UVChannel property */
	DECLARE_MAPPED_NOTIFY_PROPERTY( int, UVChannel, INT, FMeshPaintSettings::Get().UVChannel );


	/** InstanceVertexColorBytes property */
	property String^ InstanceVertexColorsText
	{
		String^ get()
		{
			String^ Text = UnrealEd::Utils::Localize( "MeshPaintWindow_InstanceVertexColorsText_NoData" );

			INT VertexColorBytes = 0;
			UBOOL bHasInstanceMaterialAndTexture = FALSE;
			MeshPaintSystem->GetSelectedMeshInfo( VertexColorBytes, bHasInstanceMaterialAndTexture );	// Out

			if( VertexColorBytes > 0 )
			{
				FLOAT VertexKiloBytes = VertexColorBytes / 1000.0f;
				Text = String::Format( UnrealEd::Utils::Localize( "MeshPaintWindow_InstanceVertexColorsText_NumBytes" ), VertexKiloBytes );
			}

			return Text;
		}
	}


	/** HasInstanceVertexColors property.  Returns true if the selected mesh has any instance vertex colors. */
	property bool HasInstanceVertexColors
	{
		bool get()
		{
			INT VertexColorBytes = 0;
			UBOOL bHasInstanceMaterialAndTexture = FALSE;
			MeshPaintSystem->GetSelectedMeshInfo( VertexColorBytes, bHasInstanceMaterialAndTexture );	// Out
			return ( VertexColorBytes > 0 );
		}
	}
	
	/** RequiresInstanceVertexColorsFixup property. Returns true if a selected mesh needs its instance vertex colors fixed-up. */
	property bool RequiresInstanceVertexColorsFixup
	{
		bool get()
		{
			return ( MeshPaintSystem->RequiresInstanceVertexColorsFixup() == TRUE );
		}
	}

	/** CanCreateInstanceMaterialAndTexture property.  Returns true a mesh is selected and we can create a new material/texture instance. */
	property bool CanCreateInstanceMaterialAndTexture
	{
		bool get()
		{
			INT VertexColorBytes = 0;
			UBOOL bHasInstanceMaterialAndTexture = FALSE;
			UBOOL bAnyValidMeshes = MeshPaintSystem->GetSelectedMeshInfo( VertexColorBytes, bHasInstanceMaterialAndTexture );	// Out
			return bAnyValidMeshes && !bHasInstanceMaterialAndTexture;
		}
	}


	/** HasInstanceMaterialAndTexture property.  Returns true if the selected mesh has a paintable material/texture instance. */
	property bool HasInstanceMaterialAndTexture
	{
		bool get()
		{
			INT VertexColorBytes = 0;
			UBOOL bHasInstanceMaterialAndTexture = FALSE;
			MeshPaintSystem->GetSelectedMeshInfo( VertexColorBytes, bHasInstanceMaterialAndTexture );	// Out
			return bHasInstanceMaterialAndTexture ? true : false;
		}
	}


	/** Radio button properties for Paint Mode */
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintModeColors, EMeshPaintMode::Type, FMeshPaintSettings::Get().PaintMode, EMeshPaintMode::PaintColors );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintModeWeights, EMeshPaintMode::Type, FMeshPaintSettings::Get().PaintMode, EMeshPaintMode::PaintWeights );


	/** ColorChannels properties */
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( WriteRed, FMeshPaintSettings::Get().bWriteRed );
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( WriteGreen, FMeshPaintSettings::Get().bWriteGreen );
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( WriteBlue, FMeshPaintSettings::Get().bWriteBlue );
	DECLARE_MAPPED_NOTIFY_BOOL_PROPERTY( WriteAlpha, FMeshPaintSettings::Get().bWriteAlpha );


	/** TotalWeightCount property */
	DECLARE_MAPPED_NOTIFY_PROPERTY( int, TotalWeightCount, INT, FMeshPaintSettings::Get().TotalWeightCount );

	/** Radio button properties for PaintWeightIndex */
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintWeightIndex1, INT, FMeshPaintSettings::Get().PaintWeightIndex, 0 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintWeightIndex2, INT, FMeshPaintSettings::Get().PaintWeightIndex, 1 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintWeightIndex3, INT, FMeshPaintSettings::Get().PaintWeightIndex, 2 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintWeightIndex4, INT, FMeshPaintSettings::Get().PaintWeightIndex, 3 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsPaintWeightIndex5, INT, FMeshPaintSettings::Get().PaintWeightIndex, 4 );

	/** Radio button properties for EraseWeightIndex */
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsEraseWeightIndex1, INT, FMeshPaintSettings::Get().EraseWeightIndex, 0 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsEraseWeightIndex2, INT, FMeshPaintSettings::Get().EraseWeightIndex, 1 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsEraseWeightIndex3, INT, FMeshPaintSettings::Get().EraseWeightIndex, 2 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsEraseWeightIndex4, INT, FMeshPaintSettings::Get().EraseWeightIndex, 3 );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsEraseWeightIndex5, INT, FMeshPaintSettings::Get().EraseWeightIndex, 4 );


	/** Radio button properties for Color View Mode */
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsColorViewModeNormal, EMeshPaintColorViewMode::Type, FMeshPaintSettings::Get().ColorViewMode, EMeshPaintColorViewMode::Normal );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsColorViewModeRGB, EMeshPaintColorViewMode::Type, FMeshPaintSettings::Get().ColorViewMode, EMeshPaintColorViewMode::RGB );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsColorViewModeAlpha, EMeshPaintColorViewMode::Type, FMeshPaintSettings::Get().ColorViewMode, EMeshPaintColorViewMode::Alpha );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsColorViewModeRed, EMeshPaintColorViewMode::Type, FMeshPaintSettings::Get().ColorViewMode, EMeshPaintColorViewMode::Red );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsColorViewModeGreen, EMeshPaintColorViewMode::Type, FMeshPaintSettings::Get().ColorViewMode, EMeshPaintColorViewMode::Green );
	DECLARE_ENUM_NOTIFY_PROPERTY( bool, IsColorViewModeBlue, EMeshPaintColorViewMode::Type, FMeshPaintSettings::Get().ColorViewMode, EMeshPaintColorViewMode::Blue );


protected:

	//color picker widgets
	MColorPickerPanel^ PaintColorPanel;
	MColorPickerPanel^ EraseColorPanel;
	FPickColorStruct* PaintColorStruct;
	FPickColorStruct* EraseColorStruct;

	/** Pointer to the mesh paint system that owns us */
	FEdModeMeshPaint* MeshPaintSystem;

	/** True if a color picker is currently visible */
	bool IsColorPickerVisible;
};



/** Static: Allocate and initialize mesh paint window */
FMeshPaintWindow* FMeshPaintWindow::CreateMeshPaintWindow( FEdModeMeshPaint* InMeshPaintSystem )
{
	FMeshPaintWindow* NewMeshPaintWindow = new FMeshPaintWindow();

	if( !NewMeshPaintWindow->InitMeshPaintWindow( InMeshPaintSystem) )
	{
		delete NewMeshPaintWindow;
		return NULL;
	}

	return NewMeshPaintWindow;
}



/** Constructor */
FMeshPaintWindow::FMeshPaintWindow()
{
	// Register to find out about other windows going modal
	GCallbackEvent->Register( CALLBACK_EditorPreModal, this );
	GCallbackEvent->Register( CALLBACK_EditorPostModal, this );
}



/** Destructor */
FMeshPaintWindow::~FMeshPaintWindow()
{
	// Unregister callbacks
	GCallbackEvent->UnregisterAll( this );

	// @todo WPF: This is probably redundant, but I'm still not sure if AutoGCRoot destructor will get
	//   called when native code destroys an object that has a non-virtual (or no) destructor

	// Dispose of WindowControl
	MeshPaintFrame.reset();
	MeshPaintPanel.reset();
}



/** Initialize the mesh paint window */
UBOOL FMeshPaintWindow::InitMeshPaintWindow( FEdModeMeshPaint* InMeshPaintSystem )
{
	WPFFrameInitStruct^ Settings = gcnew WPFFrameInitStruct;
	Settings->WindowTitle = CLRTools::LocalizeString( "MeshPaintWindow_WindowTitle" );
	Settings->bShowCloseButton = TRUE;

	MeshPaintFrame = gcnew MWPFFrame(GApp->EditorFrame, Settings, TEXT("MeshPaint"));

	PaintColorStruct.FLOATColorArray.AddItem(&(FMeshPaintSettings::Get().PaintColor));
	EraseColorStruct.FLOATColorArray.AddItem(&(FMeshPaintSettings::Get().EraseColor));
	MeshPaintPanel = gcnew MMeshPaintWindow(MeshPaintFrame.get(), InMeshPaintSystem, CLRTools::ToString(TEXT("MeshPaintWindow.xaml")), PaintColorStruct, EraseColorStruct);

	MeshPaintFrame->SetContentAndShow(MeshPaintPanel.get());

	return TRUE;
}



/** Refresh all properties */
void FMeshPaintWindow::RefreshAllProperties()
{
	MeshPaintPanel->RefreshAllProperties();
}

/** Returns true if the mouse cursor is over the mesh paint window */
UBOOL FMeshPaintWindow::IsMouseOverWindow()
{
	if( MeshPaintPanel.get() != nullptr )
	{
		FrameworkElement^ WindowContentElement = safe_cast<FrameworkElement^>( MeshPaintFrame->GetRootVisual() );
		if( WindowContentElement->IsMouseOver )
		{
			return TRUE;
		}
	}

	return FALSE;
}

/** FCallbackEventDevice: Called when a parameterless global event we've registered for is fired */
void FMeshPaintWindow::Send( ECallbackEventType Event )
{
	if( MeshPaintFrame.get() != nullptr )
	{
		FrameworkElement^ WindowContentElement = safe_cast<FrameworkElement^>( MeshPaintFrame->GetRootVisual() );

		switch ( Event )
		{
			case CALLBACK_EditorPreModal:
				WindowContentElement->IsEnabled = false;
				break;

			case CALLBACK_EditorPostModal:
				WindowContentElement->IsEnabled = true;
				break;
		}
	}
}
