
// BulletPhysicsWrapper for DarkBasic Proffessional
//Stab In The Dark Software 
//Copyright (c) 2002-2014
//http://stabinthedarksoftware.com

///#include "StdAfx.h"
#include "DBProRagDollBone.h"
#include "DBProMotionState.h"

//#include "DBPro.hpp"
#include "DBProToBullet.h"
#include "CObjectsC.h"

#ifdef WICKEDENGINE
#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
#endif

// externs to globals elsewhere
extern btDiscreteDynamicsWorld* g_dynamicsWorld;

DBProRagDollBone::DBProRagDollBone(int dbproObjectID, int dbproStartLimbID, int dbproEndLimbID, btScalar diameter, btScalar lengthmod, int collisionGroup, int collisionMask)
{
	this->dbproObjectID = dbproObjectID;
	this->dbproStartLimbID = dbproStartLimbID;
	this->dbproEndLimbID = dbproEndLimbID;
	this->diameter = diameter;
	this->lengthmod = lengthmod;
	this->collisionGroup = collisionGroup;
	this->collisionMask = collisionMask;
	CreateBone();
}

DBProRagDollBone::~DBProRagDollBone(void)
{
	g_dynamicsWorld->removeRigidBody(rigidBody);
	DBProMotionState* dbproMotionState = (DBProMotionState*)rigidBody->getMotionState();
	SAFE_DELETE(dbproMotionState); 
	SAFE_DELETE(rigidBody);
	SAFE_DELETE(m_collisionShape);
	if ( dbproRagDollBoneID > 0 ) DeleteObject(dbproRagDollBoneID);
}

enum eAxis
{
	X_AXIS,
	Y_AXIS,
	Z_AXIS
};

int DBProPrimitiveCreateCapsule(btScalar diameter, btScalar height, eAxis axis)
{
	height = (height - diameter)/2.0f;
	//rem rows must be odd
	int rows = 11; 
	int cols = 16;
	int tempSphere = FindFreeObject();
	MakeObjectSphere(tempSphere, diameter, rows, cols);
	if(axis == Z_AXIS)
		RotateObject(tempSphere,90.0,0.0,0.0);
	if(axis == X_AXIS)
		RotateObject(tempSphere,0.0,0.0,90.0);
	LockVertexDataForLimbCore(tempSphere, 0, 1);
	for(int i = 0; i < GetVertexDataVertexCount(); i++)
	{
		btScalar vertX = GetVertexDataPositionX(i);
		btScalar vertY = GetVertexDataPositionY(i);
		btScalar vertZ = GetVertexDataPositionZ(i);
		if( vertY > 0)
		{
			SetVertexDataPosition( i, vertX, vertY + height, vertZ);
		}
		else
		{
			SetVertexDataPosition( i, vertX, vertY - height, vertZ);
		}
	}
	UnlockVertexData();
	if(axis == Z_AXIS || X_AXIS)
	{
		//Remake the object to get rid of all rotations which makes it the correct axis capsule
		#ifdef DX11
		int tempMesh = FindFreeMesh();
		MakeMeshFromObject(tempMesh, tempSphere);
		DeleteObject(tempSphere);
		int capsuleObj = FindFreeObject();
		MakeObject( capsuleObj, tempMesh, 0);
		DeleteMesh(tempMesh);
		return capsuleObj;
		#else
		int tempMesh = FindFreeMesh();
		MakeMeshFromObject(tempMesh, tempSphere);
		DeleteObject(tempSphere);
		int capsuleObj = FindFreeObject();
		MakeObject( capsuleObj,tempMesh,0);
		DeleteMesh(tempMesh);
		return capsuleObj;
		#endif
	}
	return tempSphere;
}

void DBProRagDollBone::CreateBone()
{

	btScalar scaleFactor = 40.0f;///DynamicsWorldArray[0]->m_scaleFactor;
	btVector3 ObjectPosition(ObjectPositionX(dbproObjectID), ObjectPositionY(dbproObjectID),
										   ObjectPositionZ(dbproObjectID));
	btVector3 jointVec1(LimbPositionX(dbproObjectID,dbproStartLimbID), LimbPositionY(dbproObjectID,dbproStartLimbID), 
								   LimbPositionZ(dbproObjectID,dbproStartLimbID));
	btVector3 jointVec2(LimbPositionX(dbproObjectID,dbproEndLimbID), LimbPositionY(dbproObjectID,dbproEndLimbID), 
								   LimbPositionZ(dbproObjectID,dbproEndLimbID));

	//get distance between models joints
	btVector3 resultVec;
	resultVec = jointVec1 - jointVec2;

	//Get a vector which represents a line from jointVec1 to jointVec2
	//Then store this vector to calculate a min/max angle for the joint in add joint.
	btVector3 boneVec;
	boneVec = resultVec;
	boneNormVec = boneVec.normalize();

	btScalar height = resultVec.length();
	height = height * lengthmod;
	//make capsule height from distance between joints

	//Store the Total volume for mass calculations
	boneVolume = diameter * diameter * height;
	//Divide distance to get center to position capsule
	resultVec = resultVec / 2;
	btVector3 positionVec;
	positionVec = jointVec1 - resultVec;

	//LB: very small tweak to avoid feet sinking into floor
	#ifdef WICKEDENGINE
	positionVec.setY(positionVec.getY() - 5.0f);
	#endif

	//Create a World Transform for the bone
	btTransform boneTrans;
	boneTrans.setIdentity();
	boneTrans.setOrigin(positionVec / scaleFactor);

	//LB: Seems we need these objects, but can remove them from the Wicked system which slows them down
	#ifdef WICKEDENGINE
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
	WickedCall_PresetObjectCreateOnDemand(true);
	#endif
	#ifdef PRODUCTCONVERTER
	#else
	extern bool g_bDebugRagdoll;
	if (!g_bDebugRagdoll)
	{
		// not using debug capsule
		dbproRagDollBoneID = 0;

		// still need boneRotation applied to ragdoll bone
		float fAX, fAY, fAZ;
		GetAngleFromPoint (	positionVec.getX(), positionVec.getY(), positionVec.getZ(),	jointVec2.getX(), jointVec2.getY(), jointVec2.getZ(), &fAX, &fAY, &fAZ);

		// apply same rotation but quicker
		btMatrix3x3 boneRotation;
		boneRotation.setEulerYPR(btScalar(btRadians(fAX)), btScalar(btRadians(fAY)), btScalar(btRadians(fAZ)));
		boneTrans.setBasis(boneRotation);
	}
	else
	{
		//Capsule is made along the Z axis with zero rotations
		dbproRagDollBoneID = DBProPrimitiveCreateCapsule(diameter, height, Z_AXIS);
		PositionObject(dbproRagDollBoneID, positionVec.getX(), positionVec.getY(), positionVec.getZ());

		//Rotate capsule to point at second joint, now capsule has rotations.
		PointObject(dbproRagDollBoneID, jointVec2.getX(), jointVec2.getY(), jointVec2.getZ());

		//Using a matrix it takes it ZYX (uses dbproRagDollBoneID)
		btMatrix3x3 boneRotation;
		boneRotation.setEulerYPR(btScalar(btRadians(ObjectAngleZ(dbproRagDollBoneID))),
			btScalar(btRadians(ObjectAngleY(dbproRagDollBoneID))),
			btScalar(btRadians(ObjectAngleX(dbproRagDollBoneID))));
		boneTrans.setBasis(boneRotation);

		// hide it if shown
		HideObject(dbproRagDollBoneID);
	}
	#endif

	m_collisionShape = new btCapsuleShapeZ(btScalar(diameter/scaleFactor/2),btScalar((height - diameter)/scaleFactor));
	#ifdef WICKEDENGINE
	rigidBody = localCreateRigidBody(btScalar(1000.0), boneTrans, m_collisionShape, dbproRagDollBoneID, collisionGroup, collisionMask);
	#else
	rigidBody = localCreateRigidBody(btScalar(1.0), boneTrans, m_collisionShape, dbproRagDollBoneID, collisionGroup, collisionMask);
	#endif
	initialRotation = rigidBody->getWorldTransform().getBasis();

	#ifdef WICKEDENGINE
	//LB: and restore when finished creating this 'light weight' bone object
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
	WickedCall_PresetObjectCreateOnDemand(false);
	#endif

	// ZJ: the capsule object caused a large slowdown, so its not created unless in debug mode now.
	// only delete work objecty for ragdoll bone if not in debug mode
	//if (g_bDebugRagdoll == false)
	//{
	//	DeleteObject(dbproRagDollBoneID);
	//	dbproRagDollBoneID = 0;
	//}
}

btRigidBody* DBProRagDollBone::GetRigidBody()
{
	return rigidBody;
}

int DBProRagDollBone::GetStartLimbID()
{
	return dbproStartLimbID;
}
int DBProRagDollBone::GetEndLimbID()
{
	return dbproEndLimbID;
}
int DBProRagDollBone::GetObjectID()
{
	return dbproObjectID;
}
int DBProRagDollBone::GetRagDollBoneID()
{
	return dbproRagDollBoneID;
}

btVector3  DBProRagDollBone::GetNormilizedVector()
{
	return boneNormVec;
}

btRigidBody* DBProRagDollBone::localCreateRigidBody (btScalar mass, const btTransform& startTransform, btCollisionShape* shape, int objID, int collisionGroup, int collisionMask)
{
	bool isDynamic = (mass != 0.f);
	btVector3 localInertia(0,0,0);
	if (isDynamic)
	{
		shape->calculateLocalInertia(mass,localInertia);
	}
	DBProMotionState* myMotionState = new DBProMotionState(startTransform, objID);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	//add the body to the dynamics world
	if ( g_dynamicsWorld ) g_dynamicsWorld->addRigidBody(body, collisionGroup, collisionMask);
	return body;
}

void DBProRagDollBone::AddDBProLimbID(int dbproLimbID)
{
	btScalar scaleFactor = 40.0f;///DynamicsWorldArray[0]->m_scaleFactor;
	dbproLimbIDs.push_back(dbproLimbID);

	//Get limb world Position
	btVector3 limbWorldPosition(btScalar(LimbPositionX(GetObjectID(), dbproLimbID)), 
												 btScalar(LimbPositionY(GetObjectID(), dbproLimbID)), 
												 btScalar(LimbPositionZ(GetObjectID(), dbproLimbID)));
	limbWorldPosition = limbWorldPosition/scaleFactor;

	btVector3 limbOffset = limbWorldPosition - GetRigidBody()->getWorldTransform().getOrigin();

	//Store the Vector rotated
	limbOffsets.push_back(limbOffset * GetRigidBody()->getWorldTransform().getBasis());


	btVector3 objectTranslation = btVector3(ObjectPositionX(GetObjectID()),
													ObjectPositionY(GetObjectID()),
													ObjectPositionZ(GetObjectID()));
	objectTranslation = objectTranslation/scaleFactor;
	ceterOfObjectOffsets.push_back(limbWorldPosition - objectTranslation);
}



