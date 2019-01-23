#ifndef TestFxActorInstance_H__
#define TestFxActorInstance_H__

#include "FxActorInstance.h"
using namespace OC3Ent::Face;

UNITTEST( FxActorInstance, DeltaNodeCache )
{
	FxActor* pActor = new FxActor();
	CHECK( pActor );
	pActor->SetName("TestActor");

	FxFaceGraph& faceGraph = pActor->GetFaceGraph();

	FxCombinerNode* combinerNode0 = new FxCombinerNode();
	combinerNode0->SetName("testCombinerNode0");
	combinerNode0->SetMin(-10.0f);
	combinerNode0->SetMax(10.0f);
	CHECK( -10.0f == combinerNode0->GetMin() );
	CHECK( 10.0f == combinerNode0->GetMax() );
	CHECK( faceGraph.AddNode(combinerNode0) == FxTrue );

	FxDeltaNode* deltaNode0 = new FxDeltaNode();
	deltaNode0->SetName("testDeltaNode0");
	CHECK( faceGraph.AddNode(deltaNode0) == FxTrue );

	CHECK( 2 == faceGraph.GetNumNodes() );

	FxLinkFnParameters params;
	params.parameters.PushBack(1);

	CHECK( LEC_None == faceGraph.Link("testDeltaNode0", "testCombinerNode0",
		"linear", params) );
	CHECK( 1 == deltaNode0->GetNumInputLinks() );
	CHECK( deltaNode0->GetInputLink(0).GetNode() == combinerNode0 );
	CHECK( 0 == combinerNode0->GetNumInputLinks() );

	FxActorInstance* pTestInstance0 = new FxActorInstance(pActor);
	FxActorInstance* pTestInstance1 = new FxActorInstance(pActor);
	CHECK( pTestInstance0 );
	CHECK( pTestInstance1 );
	CHECK( pTestInstance0->GetActor() == pActor );
	CHECK( pTestInstance0->GetAllowNonAnimTick() == FxFalse );
	CHECK( pTestInstance1->GetActor() == pActor );
	CHECK( pTestInstance1->GetAllowNonAnimTick() == FxFalse );

	pTestInstance0->SetAllowNonAnimTick(FxTrue);
	pTestInstance1->SetAllowNonAnimTick(FxTrue);
	CHECK( pTestInstance0->GetAllowNonAnimTick() == FxTrue );
	CHECK( pTestInstance1->GetAllowNonAnimTick() == FxTrue );

	CHECK( pTestInstance0->SetRegister("testCombinerNode0", 1.0f, VO_Replace) == FxTrue );
	pTestInstance0->Tick(0.0, -1.0f);
	pTestInstance0->BeginFrame();
	CHECK( 0.0f == deltaNode0->GetValue() );
	pTestInstance0->EndFrame();

	CHECK( pTestInstance1->SetRegister("testCombinerNode0", 2.0f, VO_Replace) == FxTrue );
	pTestInstance1->Tick(0.0, -1.0f);
	pTestInstance1->BeginFrame();
	CHECK( 0.0f == deltaNode0->GetValue() );
	pTestInstance1->EndFrame();

	CHECK( pTestInstance0->SetRegister("testCombinerNode0", 2.5f, VO_Replace) == FxTrue );
	pTestInstance0->Tick(0.0, -1.0f);
	pTestInstance0->BeginFrame();
	CHECK( 1.5f == deltaNode0->GetValue() );
	pTestInstance0->EndFrame();

	CHECK( pTestInstance1->SetRegister("testCombinerNode0", 5.0f, VO_Replace) == FxTrue );
	pTestInstance1->Tick(0.0, -1.0f);
	pTestInstance1->BeginFrame();
	CHECK( 3.0f == deltaNode0->GetValue() );
	pTestInstance1->EndFrame();

	CHECK( pTestInstance0->SetRegister("testCombinerNode0", 8.25f, VO_Replace) == FxTrue );
	pTestInstance0->Tick(0.0, -1.0f);
	pTestInstance0->BeginFrame();
	CHECK( 5.75f == deltaNode0->GetValue() );
	pTestInstance0->EndFrame();

	CHECK( pTestInstance1->SetRegister("testCombinerNode0", 1.0f, VO_Replace) == FxTrue );
	pTestInstance1->Tick(0.0, -1.0f);
	pTestInstance1->BeginFrame();
	CHECK( -4.0f == deltaNode0->GetValue() );
	pTestInstance1->EndFrame();

	delete pTestInstance0;
	pTestInstance0 = NULL;
	delete pTestInstance1;
	pTestInstance1 = NULL;
	delete pActor;
	pActor = NULL;
}

UNITTEST( FxActorInstance, Registers )
{
	FxActor* pActor = new FxActor();
	CHECK( pActor );
	pActor->SetName("TestActor");

	FxCombinerNode* combinerNode0 = new FxCombinerNode();
	combinerNode0->SetName("testCombinerNode0");
	CHECK( pActor->GetFaceGraph().AddNode(combinerNode0) == FxTrue );

	FxCombinerNode* combinerNode1 = new FxCombinerNode();
	combinerNode1->SetName("testCombinerNode1");
	CHECK( pActor->GetFaceGraph().AddNode(combinerNode1) == FxTrue );

	CHECK( pActor->GetFaceGraph().GetNumNodes() == 2 );
	CHECK( pActor->GetFaceGraph().CountNodes() == 2 );

	FxActorInstance* pActorInstance = new FxActorInstance(pActor);
	CHECK( pActorInstance );
    CHECK( pActorInstance->GetActor() == pActor );
	CHECK( pActorInstance->GetAllowNonAnimTick() == FxFalse );

	pActorInstance->SetAllowNonAnimTick(FxTrue);
	CHECK( pActorInstance->GetAllowNonAnimTick() == FxTrue );

	CHECK( pActorInstance->SetRegister("testCombinerNode0", 1.0f, VO_Replace) == FxTrue );
	CHECK( pActorInstance->SetRegister("testCombinerNode1", 1.0f, VO_Replace) == FxTrue );
	CHECK( pActorInstance->SetRegister("nodeDoesNotExist", 1.0f, VO_Replace) == FxFalse );

	pActorInstance->Tick(0.0, -1.0f);

	pActorInstance->BeginFrame();
	pActorInstance->EndFrame();

	CHECK( pActorInstance->GetRegister("testCombinerNode0") == 1.0f );
	CHECK( pActorInstance->GetRegister("testCombinerNode1") == 1.0f );
	CHECK( pActorInstance->GetRegister("nodeDoesNotExist") == 0.0f );

	CHECK( pActorInstance->SetRegister("testCombinerNode0", 0.5f, VO_Replace) == FxTrue );
	CHECK( pActorInstance->SetRegister("testCombinerNode1", 0.2f, VO_Replace) == FxTrue );
	CHECK( pActorInstance->SetRegister("nodeDoesNotExist", 0.1f, VO_Replace) == FxFalse );

	pActorInstance->Tick(0.0, -1.0f);

	pActorInstance->BeginFrame();
	pActorInstance->EndFrame();

	CHECK( pActorInstance->GetRegister("testCombinerNode0") == 0.5f );
	CHECK( pActorInstance->GetRegister("testCombinerNode1") == 0.2f );
	CHECK( pActorInstance->GetRegister("nodeDoesNotExist") == 0.0f );

	delete pActorInstance;
	pActorInstance = NULL;
	delete pActor;
	pActor = NULL;
}

#endif