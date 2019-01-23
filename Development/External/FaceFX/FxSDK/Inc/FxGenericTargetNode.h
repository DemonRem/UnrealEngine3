//------------------------------------------------------------------------------
// This class implements a generic target node in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxGenericTargetNode_H__
#define FxGenericTargetNode_H__

#include "FxPlatform.h"
#include "FxArchive.h"
#include "FxFaceGraphNode.h"

namespace OC3Ent
{

namespace Face
{

// Forward declare FxGenericTargetNode.
class FxGenericTargetNode;

/// A generic target proxy used in conjunction with an FxGenericTargetNode.
/// An FxGenericTargetProxy cannot be instantiated because it is an abstract
/// class.  Instead, the programmer should derive a new target proxy type
/// from FxGenericTargetProxy and override the Copy(), Update(), and Destroy()
/// functions.  In most cases, FxGenericTargetProxy should contain only 
/// references and pointers to objects.  However, if you have a valid copy 
/// constructor for your object, and properly implement Copy(), objects may be 
/// contained in the target proxy.  Just note that it is strongly discouraged 
/// to have the target proxy contain anything more than pointers and references.
/// FxGenericTargetProxy objects are not serialized along with their 
/// parent FxGenericTargetNode object, so they must be manually created and 
/// hooked up at %Face Graph initialization time.
/// \ingroup faceGraph
class FxGenericTargetProxy : public FxObject
{
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxGenericTargetProxy, FxObject)
public:
	/// Generic target nodes are our friends.
	friend class FxGenericTargetNode;

	/// Copies the properties from the other generic target proxy into this one.
	/// \note When overriding this function, ensure all properties that
	/// have been added in the derived class are copied.
	/// \param pOther The source for the copy operation.
	virtual void Copy( FxGenericTargetProxy* pOther ) = 0;

	/// Updates the generic target.
	virtual void Update( FxReal value ) = 0;

	/// Returns the FxGenericTargetNode that controls this generic target proxy.
	FxGenericTargetNode* GetParentGenericTargetNode( void );

	/// Sets the FxGenericTargetNode that controls this generic target proxy.
	/// This is automatically called by FxGenericTargetNode::SetGenericTargetProxy().
	/// \param pParentGenericTargetNode A pointer to node containing this proxy object.
	void SetParentGenericTargetNode( FxGenericTargetNode* pParentGenericTargetNode );

protected:
	/// The FxGenericTargetNode that controls this generic target proxy.
	FxGenericTargetNode* _pParentGenericTargetNode;

	/// Constructor.
	/// The constructor and destructor are protected to prevent actually 
	/// instantiating a base FxGenericTargetProxy.
	FxGenericTargetProxy();
	/// Destructor.
	virtual ~FxGenericTargetProxy();
	/// Destroys the generic target proxy.  This must be overridden to call
	/// the appropriate destructor.
	virtual void Destroy( void ) = 0;
};

/// A generic target node in an FxFaceGraph.
/// A generic target node is a node in the %Face Graph that allows the programmer
/// to easily drive any arbitrary target by simply plugging in an appropriate
/// FxGenericTargetProxy object.  FxGenericTargetProxy objects are not 
/// serialized along with their parent FxGenericTargetNode object, so they must
/// be manually created and hooked up at %Face Graph initialization time.  It is
/// also the programmer's responsibility to make sure that UpdateTarget() is
/// called once per frame between the BeginFrame() / EndFrame() pair for each
/// FxGenericTargetNode that should be updated.
/// \ingroup faceGraph
class FxGenericTargetNode : public FxFaceGraphNode
{
	/// Declare the class.
	FX_DECLARE_CLASS(FxGenericTargetNode, FxFaceGraphNode)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxGenericTargetNode)
public:
	/// Clones the generic target node.
	virtual FxFaceGraphNode* Clone( void );

	/// Copies the data from this object into the other object.
	virtual void CopyData( FxFaceGraphNode* pOther );

	/// Returns the generic target proxy associated with this node.
	FxGenericTargetProxy* GetGenericTargetProxy( void );

	/// Sets the generic target proxy associated with this node.
	/// \note that the pGenericTargetProxy should be created on the heap with
	/// operator \p new and FxGenericTargetNode assumes control of the memory.
	/// \param pGenericTargetProxy The proxy object to associate with this node.
	virtual void SetGenericTargetProxy( FxGenericTargetProxy* pGenericTargetProxy );

	/// Returns FxTrue if the game engine should perform a "re-link" step during
	/// the next render.
	virtual FxBool ShouldLink( void ) const;
	/// Sets whether or not the game engine should perform a "re-link" step 
	/// during the next render.
	virtual void SetShouldLink( FxBool shouldLink );

	/// Updates the generic target.  This must be called once per-frame so that
	/// the target this generic target node is controlling is updated correctly.
	void UpdateTarget( void );

	/// Serializes an FxGenericTargetNode to an archive.
	virtual void Serialize( FxArchive& arc );

protected:
	/// The generic target proxy.
	FxGenericTargetProxy* _pGenericTargetProxy;
	/// FxTrue if the game engine should perform a "re-link" step during the 
	/// next render.
	FxBool _shouldLink;

	/// Constructor.
	/// The constructor and destructor are protected to prevent actually 
	/// instantiating a base FxGenericTargetNode.
	FxGenericTargetNode();
	/// Destructor.
	virtual ~FxGenericTargetNode();

	/// Deletes the generic target proxy.
	void _deleteGenericTargetProxy( void );
};

} // namespace Face

} // namespace OC3Ent

#endif
