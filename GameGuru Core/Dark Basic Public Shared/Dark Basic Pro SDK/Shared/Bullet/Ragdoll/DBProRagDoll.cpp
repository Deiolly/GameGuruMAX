// BulletPhysicsWrapper for DarkBasic Proffessional
// Stab In The Dark Software 
// Copyright (c) 2002-2014
// http://stabinthedarksoftware.com

///#include "StdAfx.h"
#include "DBProRagDoll.h"
#include "DBProToBullet.h"
#include "DBProMotionState.h"
#include "DBProJoints.h"
#include "DBProJointManager.h"
#include "BT2DX.h"

#include "CObjectsC.h"
#include ".\..\..\Error\CError.h"

#ifdef WICKEDENGINE
#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
#endif

// externs to globals elsewhere
extern btDiscreteDynamicsWorld* g_dynamicsWorld;
extern DBProJointManager* jointManager;
extern float gSc;

//Global External variables
DBProRagDoll* currentDBProRagDoll = NULL;

DBProRagDoll::DBProRagDoll(int ragdollID) : BaseItem(ragdollID)
{
	//070714 - missing init vars
	m_modelTotalWeight=0;
	m_modelTotalVolume=0;
	m_modelTotalDensity=0;

	m_bNeedsUpdateForCulling = false;
#ifndef WICKEDENGINE
	m_linearDamping = 0.05f;
	m_angularDamping = 0.85f;
#else
	m_linearDamping = 0.5f;
	m_angularDamping = 0.95f;
#endif
	m_linearSleepingThresholds = 1.6f;
	m_angularSleepingThresholds = 2.5f;
	m_deactivationTime = 0.8f;
	
	m_bIsStatic = false;
	m_fRadius = 0;
	m_fScaledRadius = 0;
	m_fScaledLargestRadius = 0;
	m_bNeedsUpdateForCulling = false;

	sObject* pObject = GetObjectData(ragdollID);
	//SetCullingRadius(pObject->collision.fRadius, pObject->collision.fScaledRadius, pObject->collision.fScaledLargestRadius);
	pObject->collision.fRadius=0;
	pObject->collision.fScaledRadius=0;
	pObject->collision.fScaledLargestRadius=0;

#ifdef WICKEDENGINE
	// Find the frame just after the root i.e. the frame that animates the skeleton, but not the world orientation.
	m_iAnimateFromJoint = 2;
	for (int iF = 0; iF < pObject->iFrameCount; iF++)
	{
		sFrame* pFrame = pObject->ppFrameList[iF];
		if (strcmp(pFrame->szName, "Bip01") == NULL)
		{
			m_iAnimateFromJoint = iF;
			break;
		}
	}
#endif
}

DBProRagDoll::~DBProRagDoll()
{
	if ( jointManager )
	{
		for (int i = 0; i < m_joints.size(); i++)
		{
			jointManager->DeleteJoint(m_joints[i]);
		}
	}
	for ( int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		SAFE_DELETE(m_ragDollBoneArray[i]);
	}
	m_ragDollBoneArray.clear();
}

void DBProRagDoll::ResetObjectParametersForAnimation()
{
	sObject* pObject = GetObjectData(m_id);
	if(pObject)
	{
		pObject->position.bCustomBoneMatrix = false;
		pObject->bUpdateOverallBounds = true;
	}
}

void DBProRagDoll::ResetObjectParametersForCulling()
{
	if(!m_bNeedsUpdateForCulling)
		return;
	m_bNeedsUpdateForCulling = false;
	sObject* pObject = GetObjectData(m_id);

	// all meshes in object
	for ( int iFrame = 0; iFrame < pObject->iFrameCount; iFrame++ )
	{
		// get a pointer to the frame
		sFrame* pFrame = pObject->ppFrameList [ iFrame ];

		sMesh* pMesh = pFrame->pMesh;
		if(pMesh!=NULL)
		{
			CalculateMeshBounds(pMesh);
			btTransform transform = DBProToBullet::GetTransform(m_id);
			transform.setOrigin(btVector3(0.0,0.0,0.0));
			btVector3 btVecCenter = transform * BT2DX::DX_VECTOR3_2BT(pMesh->Collision.vecCentre);
			GGVECTOR3 vecCenter = BT2DX::BT2DX_VECTOR3(btVecCenter);
			//
			btVector3 btVecMin =transform * BT2DX::DX_VECTOR3_2BT(pMesh->Collision.vecMin);
			GGVECTOR3 vecMin = BT2DX::BT2DX_VECTOR3(btVecMin);
			//
			btVector3 btVecMax= transform * BT2DX::DX_VECTOR3_2BT(pMesh->Collision.vecMax);
			GGVECTOR3 vecMax = BT2DX::BT2DX_VECTOR3(btVecMax);
			//
			pObject->collision.vecCentre = vecCenter;//pMesh->Collision.vecCentre;
			pObject->collision.vecColCenter = vecCenter;//pMesh->Collision.vecCentre;
			pObject->collision.vecMin = vecMin;//pMesh->Collision.vecMin;
			pObject->collision.vecMax = vecMax;//pMesh->Collision.vecMax;

			pObject->collision.fRadius = 0;//pMesh->Collision.fRadius;
			pObject->collision.fLargestRadius = 0;//pMesh->Collision.fRadius;
			pObject->collision.fScaledRadius = 0;//pMesh->Collision.fRadius;
			pObject->collision.fScaledLargestRadius = 0;//->Collision.fRadius;
			pObject->collision.bColCenterUpdated = true;
		}
	}
	//FHB:Leave for debugging
	//ShowObjectBounds(m_id, 0);
}

void DBProRagDoll::SetTotalWeight(btScalar totalWeight)
{
	m_modelTotalWeight = totalWeight;
}

void DBProRagDoll::SetFriction(btScalar friction)
{
	for ( int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		btRigidBody* body = m_ragDollBoneArray[i]->GetRigidBody();
		if(body)
		{
			body->setFriction(friction);
			//body->setAnisotropicFriction ( btVector3(friction,friction,friction), 1 );
		}
	}
}

bool DBProRagDoll::IsStatic()
{
	return m_bIsStatic;
}

void DBProRagDoll::SetStatic(bool bStatic)
{
	m_bIsStatic = bStatic;
	for ( int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		if(bStatic)
		{
			//g_dynamicsWorld->SetRigidBodyMass(m_ragDollBoneArray[i]->GetRigidBody(), 0.0);
			btRigidBody* body = m_ragDollBoneArray[i]->GetRigidBody();
			btScalar mass = 0.0f;
			if(body)
			{
				btVector3 localInertia(0.0,0.0,0.0);
				body->getCollisionShape()->calculateLocalInertia(mass,localInertia);
				body->setMassProps(mass, localInertia);
			}
		}
		else
		{
			btScalar mass = m_modelTotalDensity * m_ragDollBoneArray[i]->boneVolume;
			//DynamicsWorldArray[0]->SetRigidBodyMass(m_ragDollBoneArray[i]->GetRigidBody(),mass);
			btRigidBody* body = m_ragDollBoneArray[i]->GetRigidBody();
			if(body)
			{
				btVector3 localInertia(0.0,0.0,0.0);
				body->getCollisionShape()->calculateLocalInertia(mass,localInertia);
				body->setMassProps(mass, localInertia);
			}
		}
	}
}

btRigidBody* DBProRagDoll::localCreateRigidBody (btScalar mass, const btTransform& startTransform, btCollisionShape* shape, int objID, int collisionGroup, int collisionMask)
{
	bool isDynamic = (mass != 0.f);
	btVector3 localInertia(0,0,0);
	if (isDynamic)
		shape->calculateLocalInertia(mass,localInertia);
	DBProMotionState* myMotionState = new DBProMotionState(startTransform, objID);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	g_dynamicsWorld->addRigidBody(body, collisionGroup, collisionMask);
	return body;
}

int DBProRagDoll::AddBone(int objID, int limbID1, int limbID2, btScalar diameter, int collisionGroup, int collisionMask)
{
	m_ragDollBoneArray.push_back(new DBProRagDollBone(objID, limbID1, limbID2, diameter, 1.0f, collisionGroup, collisionMask));

	return m_ragDollBoneArray.size()-1;
}

int DBProRagDoll::AddBone(int objID, int limbID1, int limbID2, btScalar diameter, float lengthmod, int collisionGroup, int collisionMask)
{
	m_ragDollBoneArray.push_back(new DBProRagDollBone(objID, limbID1, limbID2, diameter, lengthmod, collisionGroup, collisionMask));

	return m_ragDollBoneArray.size()-1;
}

void DBProRagDoll::AddHingeJoint(int boneIndex1, int boneIndex2, int limbID, btVector3& jointRotation,
																					btScalar minRotation, btScalar maxRotation)
{
	btScalar scaleFactor = 40.0f;///DynamicsWorldArray[0]->m_scaleFactor;
	btHingeConstraint* hingeC;
	btTransform localATrans, localBTrans, modelTrans, jointTrans, limbRotationTrans;
	btMatrix3x3 modelMatrix, jointMatrix, limbMatrix;
	int objectID = m_id;
	btVector3 modelPosition(ObjectPositionX(objectID), ObjectPositionY(objectID), ObjectPositionZ(objectID));
	modelPosition = modelPosition/scaleFactor;

	//Create TransForm for DBPro Model do not include position.
	//We we use setEulerYPR for DBpro rotations passed in, the order is ZYX
	modelMatrix.setEulerYPR(btRadians(ObjectAngleZ(objectID)),
												btRadians(ObjectAngleY(objectID)),
													btRadians(ObjectAngleX(objectID)));
	modelTrans.setIdentity();
	modelTrans.setBasis(modelMatrix);

	//Create Transform for Joint with Position and Rotation
	btVector3 jointPosition(LimbPositionX(objectID,limbID), LimbPositionY(objectID,limbID), LimbPositionZ(objectID,limbID));
	jointPosition= jointPosition/scaleFactor;

	//When we use setEulerZYX here it needs to be YXZ
	jointMatrix.setEulerZYX(btRadians(jointRotation.getY()), btRadians(jointRotation.getX()), btRadians(jointRotation.getZ()));
	jointTrans.setIdentity();
	jointTrans.setBasis(jointMatrix);

	//FHB: Removed, not needed when we handel a frame of animation, might be needed if model has no animation.
	//If the model is rotated from its original orientation.
	//the joint will then have the same orientation as the models orientation.
	//jointTrans = modelTrans * jointTrans;

	//Adjustment based on the joints rotations at a frame of animation 

	//Get current limb rotation from animation and convert to radians
	btVector3 limbRotation(btRadians(LimbDirectionX(objectID,limbID)), btRadians(LimbDirectionY(objectID,limbID)), 
																		btRadians(LimbDirectionZ(objectID,limbID)));
	//Create Rotation only Transform for Models limb, No position
	//We use setEulerYPR for DBpro rotations passed in, the order is ZYX
	limbMatrix.setEulerYPR(limbRotation.getZ(), limbRotation.getY(), limbRotation.getX());//This rotation works for the models limbs rotations.
	limbRotationTrans.setIdentity();
	limbRotationTrans.setBasis(limbMatrix);

	//Rotate the joints transform with the limbs rotation
	jointTrans = limbRotationTrans * jointTrans ;

	jointTrans.setOrigin(jointPosition);
	
	//Creates local transforms for each side of joint using our adjusted joint transform
	DBProJoints::CreateLocalTransforms(m_ragDollBoneArray[boneIndex1]->GetRigidBody(),
																m_ragDollBoneArray[boneIndex2]->GetRigidBody(),
																	jointTrans, localATrans, localBTrans);

	btScalar minLimit = btRadians(minRotation);
	btScalar maxLimit = btRadians(maxRotation);

	//Get the Angle between the two bone capsules
	btVector3 bone1Norm, bone2Norm;
	bone1Norm = m_ragDollBoneArray[boneIndex1]->GetNormilizedVector();
	bone2Norm = m_ragDollBoneArray[boneIndex2]->GetNormilizedVector();
	btScalar ang = bone1Norm.angle(bone2Norm);

	//Subtract this angle from the limits.
	minLimit = minLimit - abs(ang);
	maxLimit = maxLimit - abs(ang);

	hingeC = DBProJoints::CreateHingeConstraint(m_ragDollBoneArray[boneIndex1]->GetRigidBody(), 
																				m_ragDollBoneArray[boneIndex2]->GetRigidBody(),
																					localATrans, localBTrans, minLimit, maxLimit,true);
	int jointID = jointManager->AddJoint(hingeC);
	m_joints.push_back(jointID);
	hingeC = NULL;
}

void DBProRagDoll::AddTwistConstraint(int boneIndex1, int boneIndex2, int limbID, btVector3& jointRotation, btVector3& jointlimits)
{
	btScalar scaleFactor = 40.0f;///DynamicsWorldArray[0]->m_scaleFactor;
	btConeTwistConstraint* coneC; 
	btTransform localATrans, localBTrans, modelTrans, jointTrans, limbRotationTrans;
	btMatrix3x3 modelMatrix, jointMatrix, limbMatrix;
	int objectID = m_id;
	//Create TransForm for DBPro Model do not include position.
	//We we use setEulerYPR for DBpro rotations passed in, the order is ZYX
	btVector3 modelPosition(ObjectPositionX(objectID), ObjectPositionY(objectID), ObjectPositionZ(objectID));
	modelPosition = modelPosition/scaleFactor;
	modelMatrix.setEulerYPR(btRadians(ObjectAngleZ(objectID)),
											btRadians(ObjectAngleY(objectID)),
											btRadians(ObjectAngleX(objectID)));
	modelTrans.setIdentity();
	modelTrans.setBasis(modelMatrix);

	//Create Transform for Joint, When we use setEulerYPR here it needs to be YXZ
	btVector3 jointPosition(LimbPositionX(objectID,limbID), LimbPositionY(objectID,limbID), LimbPositionZ(objectID,limbID));
	jointPosition= jointPosition/scaleFactor;

	//When we use setEulerZYX here it needs to be YXZ
	jointMatrix.setEulerZYX(btRadians(jointRotation.getY()), btRadians(jointRotation.getX()), btRadians(jointRotation.getZ()));
	jointTrans.setIdentity();
	jointTrans.setBasis(jointMatrix);

	//FHB: Removed, not needed when we handel a frame of animation, might be needed if model has no animation.
	//If the model is rotated from its original orientation.
	//the joint will then have the same orientation as the models orientation.
	//jointTrans = jointTrans * modelTrans;
	
	//Adjustment based on the joints rotations at a frame of animation 

	//Get current limb rotation from animation and conver to radians
	btVector3 limbRotation(btRadians(LimbDirectionX(objectID,limbID)), btRadians(LimbDirectionY(objectID,limbID)), 
																		btRadians(LimbDirectionZ(objectID,limbID)));
	//Create Rotation only Transform for Models limb No position
	//We use setEulerYPR for DBpro rotations passed in, the order is ZYX
	limbMatrix.setEulerYPR(limbRotation.getZ(), limbRotation.getY(), limbRotation.getX());//This rotation works for the models limbs rotations.
	limbRotationTrans.setIdentity();
	limbRotationTrans.setBasis(limbMatrix);

	//Rotate the joints transform with the limbs rotation
	jointTrans = limbRotationTrans * jointTrans ;

	jointTrans.setOrigin(jointPosition);

	//Creates local transforms for each side of joint using our adjusted joint transform
	DBProJoints::CreateLocalTransforms( m_ragDollBoneArray[boneIndex1]->GetRigidBody(),
																m_ragDollBoneArray[boneIndex2]->GetRigidBody(),
																	jointTrans, localATrans, localBTrans);

	btScalar swingSpan1 = btRadians(jointlimits.getZ());
	btScalar swingSpan2 = btRadians(jointlimits.getY());
	btScalar twistSpan = btRadians(jointlimits.getX());

	coneC =  DBProJoints::CreateTwistConstraint(m_ragDollBoneArray[boneIndex1]->GetRigidBody(), 
																				m_ragDollBoneArray[boneIndex2]->GetRigidBody(),
																					localATrans, localBTrans, swingSpan1, swingSpan2, twistSpan, true);

	//TODO: Figure out how to handel limits of cone twist at a frame of animation.
	//btScalar twistAng = coneC->getTwistAngle();

	int jointID = jointManager->AddJoint(coneC);
	m_joints.push_back(jointID);
	coneC = NULL;
}


int DBProRagDoll::GetBoneObjID(int boneIndex) 
{
	return (boneIndex >= 0 && boneIndex < m_ragDollBoneArray.size()) ? m_ragDollBoneArray[boneIndex]->GetRagDollBoneID() : -1; 
} 

bool DBProRagDoll::IsBoneObject(int objectID)
{
	for(int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		if(m_ragDollBoneArray[i]->GetObjectID()==objectID)
		{
			return true;
		}
	}
	return false;
}

void DBProRagDoll::Finalize()
{
	int objectID = m_id;
	sObject* pObject = GetObjectData(objectID);

	for (int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		m_modelTotalVolume += m_ragDollBoneArray[i]->boneVolume;
	}
	//Calculate density of ragdoll
	m_modelTotalDensity = m_modelTotalWeight / m_modelTotalVolume;

	//Set the mass of all the Rag doll bones to the calculated mass.
	for (int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		btScalar mass = m_modelTotalDensity * m_ragDollBoneArray[i]->boneVolume;
		btRigidBody* body = m_ragDollBoneArray[i]->GetRigidBody();
		if(body)
		{
			btVector3 localInertia(0.0,0.0,0.0);
			body->getCollisionShape()->calculateLocalInertia(mass,localInertia);
			body->setMassProps(mass, localInertia);
		}
		m_ragDollBoneArray[i]->GetRigidBody()->setFriction(0.75f);
		m_ragDollBoneArray[i]->GetRigidBody()->setDamping(m_linearDamping, m_angularDamping);
		m_ragDollBoneArray[i]->GetRigidBody()->setDeactivationTime(m_deactivationTime);
		m_ragDollBoneArray[i]->GetRigidBody()->setSleepingThresholds(m_linearSleepingThresholds, m_angularSleepingThresholds);
	}
	//Set the object model to use CustomBoneMatrix for Rag doll
	//We can now move the models limbs with matCombined
	pObject->position.bCustomBoneMatrix = true;
	// This prevents the object from being culled when we move the limbs of the object.
	//Copied from DBPro Source Code
	//SetCullingRadius(pObject->collision.fRadius, pObject->collision.fScaledRadius, pObject->collision.fScaledLargestRadius);
	pObject->collision.fRadius = 0;
	pObject->collision.fScaledRadius = 0;
	pObject->collision.fScaledLargestRadius = 0;
}

void DBProRagDoll::SetCullingRadius(float fRadius, float fScaledRadius, float fScaledLargestRadius)
{
	m_fRadius = fRadius;
	m_fScaledRadius = fScaledRadius;
	m_fScaledLargestRadius = fScaledLargestRadius;
}

void DBProRagDoll::GetCullingRadius(float &outfRadius, float &outfScaledRadius, float &outfScaledLargestRadius)
{
	outfRadius = m_fRadius;
	outfScaledRadius = m_fScaledRadius;
	outfScaledLargestRadius = m_fScaledLargestRadius;
}

bool  DBProRagDoll::IsSleeping()
{
	bool bIsSleeping = true;
	for (int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		bIsSleeping = bIsSleeping && !m_ragDollBoneArray[i]->GetRigidBody()->isActive();
	}
	return bIsSleeping;
}

void DBProRagDoll::Activate()
{
	for (int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		m_ragDollBoneArray[i]->GetRigidBody()->activate();
	}
}

void DBProRagDoll::ApplyForce(int iLimbNumber, btVector3& Start, btVector3& End, float fForceValue)
{
	for (int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		btRigidBody* pBody = m_ragDollBoneArray[i]->GetRigidBody();
		btVector3 thisPos = pBody->getWorldTransform().getOrigin();
		btVector3 relPos = btVector3(0,0,0);//End - thisPos;
		btVector3 force = (End-Start);
		force = force / force.length() / gSc;
		force = force * (fForceValue/gSc);
		if ( iLimbNumber==-1 )
		{
			pBody->applyForce(force, relPos);
		}
		else
		{
			for (int j = 0; j < m_ragDollBoneArray[i]->limbOffsets.size(); j++)
			{
				if ( iLimbNumber==m_ragDollBoneArray[i]->dbproLimbIDs[j] )
				{
					pBody->applyForce(force, relPos);
				}
			}
		}
	}
}

/* not needed
void CalculatePureRotationsFromCombed (sFrame* pFrame)
{
	if (pFrame)
	{
		// work out pure rotation from local matrix (matCombined)
		GGVECTOR3 vecPoint = GGVECTOR3(0, 0, 0);
		GGVec3TransformCoord(&vecPoint, &vecPoint, &pFrame->matCombined);
		GGMATRIX matPureRot;
		GGVECTOR3 eye = GGVECTOR3(0,0,0);
		GGVECTOR3 at = vecPoint;
		GGVECTOR3 up = GGVECTOR3(0,1,0);
		GGMatrixLookAtLH(&matPureRot, &eye, &at, &up);
		pFrame->matCombined = matPureRot;
		if (pFrame->pChild) CalculatePureRotationsFromCombed(pFrame->pChild);
		if (pFrame->pSibling) CalculatePureRotationsFromCombed(pFrame->pSibling);
	}
}

bool g_bInitialQuatCollection = false;
GGVECTOR4 g_ragdollWorldQuats[100];
GGVECTOR4 g_poseWorldQuats[100];
GGVECTOR4 g_ragdolltoPoseQuats[100];
GGVECTOR4 g_ragdollQuats[100];
GGVECTOR4 g_ragdollInitialQuats[100];
GGVECTOR4 g_ragdollDifferenceQuats[100];

void CalculateLocalQuatFromWorldQuat(sFrame* pFrame, GGVECTOR4 poseParentWorld, GGVECTOR4 ragdollParentWorld)
{
	if (pFrame)
	{
		// make pose world from local pose
		GGVECTOR4 localInitialPoseQuat = GGVECTOR4(pFrame->matOriginal._11, pFrame->matOriginal._12, pFrame->matOriginal._13, pFrame->matOriginal._14);
		WickedCall_CalculateWorldQuat(&poseParentWorld, &localInitialPoseQuat, &g_poseWorldQuats[pFrame->iID]);

		// need local rot of this frame, so take this world and deduct parents world
		GGVECTOR4 thisWorld = g_poseWorldQuats[pFrame->iID];
		GGVECTOR4 localPoseRot;
		WickedCall_CalculateQuatDiff(&poseParentWorld, &thisWorld, &localPoseRot);

		// need relative ragdoll rot, so take ragdoll world and deduct parent ragdoll world
		GGVECTOR4 localRagdollRot;
		WickedCall_CalculateQuatDiff(&ragdollParentWorld, &g_ragdollWorldQuats[pFrame->iID], &localRagdollRot);

		// some idea that we handle local ragdoll rot through the orientation of the local pose rot
		//WickedCall_CalculateQuatDiff(&localPoseRot, &localRagdollRot, &localRagdollRot);

		// work out final relative local rot for this frame
		if (g_bInitialQuatCollection == false)
		{
			// work out difference between local pose and local ragdoll
			GGVECTOR4 localDifferenceToBridgeGap;
			//WickedCall_CalculateQuatDiff(&localPoseRot, &localRagdollRot, &localDifferenceToBridgeGap);
			WickedCall_CalculateQuatDiff(&localRagdollRot, &localPoseRot, &localDifferenceToBridgeGap);

			// initial bridge gap rot to be used later
			g_ragdolltoPoseQuats[pFrame->iID] = localDifferenceToBridgeGap;
			g_ragdollQuats[pFrame->iID] = GGVECTOR4(0, 0, 0, 1);
		}
		else
		{
			// the local rotation that represents the change in ragdoll compared to pose 
			WickedCall_CalculateQuatDiff(&g_ragdolltoPoseQuats[pFrame->iID], &localRagdollRot, &g_ragdollQuats[pFrame->iID]);
		}

		// do all frames in object
		if (pFrame->pChild) CalculateLocalQuatFromWorldQuat(pFrame->pChild, g_poseWorldQuats[pFrame->iID], g_ragdollWorldQuats[pFrame->iID]);
		if (pFrame->pSibling) CalculateLocalQuatFromWorldQuat(pFrame->pSibling, poseParentWorld, ragdollParentWorld);
	}
}
*/

void CalculateRelativeMatricesFromWorld (sFrame* pParentFrame, sFrame* pFrame)
{
	if (pFrame)
	{
		if (pParentFrame)
		{
			float fDet = 0.0f;
			GGMATRIX matMatrixWorldAtThisPointInv;
			GGMatrixInverse (&matMatrixWorldAtThisPointInv, &fDet, &pParentFrame->matCombined);
			GGMatrixMultiply (&pFrame->matTransformed, &pFrame->matCombined, &matMatrixWorldAtThisPointInv);
		}
		else
		{
			GGMatrixIdentity (&pFrame->matTransformed);
		}
		if (pFrame->pChild) CalculateRelativeMatricesFromWorld(pFrame, pFrame->pChild);
		if (pFrame->pSibling) CalculateRelativeMatricesFromWorld(pParentFrame, pFrame->pSibling);
	}
}

void DBProRagDoll::Update() 
{
	m_bNeedsUpdateForCulling = true;
	int objectID = m_id;
	sObject* pObject = GetObjectData(objectID);

	// Turn Culling Off
	pObject->collision.fRadius = 0;
	pObject->collision.fScaledRadius = 0;
	pObject->collision.fScaledLargestRadius = 0;

	btScalar scaleFactor = 40.0f;///DynamicsWorldArray[0]->m_scaleFactor;

	//Create Transform for DBpro Model,needed for when model is not positioned at origin
	btTransform DBProModelTrans, DBProModelTransRot, limbInitalRotationTrans;
	btMatrix3x3 DBProModelRotation;
	DBProModelTrans.setIdentity();
	DBProModelTransRot.setIdentity();
	limbInitalRotationTrans.setIdentity();

	DBProModelRotation.setEulerYPR(btRadians(ObjectAngleZ(objectID)),
															btRadians(ObjectAngleY(objectID)),
																btRadians(ObjectAngleX(objectID)));
	DBProModelTransRot.setBasis(DBProModelRotation);

	btVector3 modelPosition = -1 * BT2DX::DX_VECTOR3_2BT(pObject->position.vecPosition);
	modelPosition = modelPosition/scaleFactor;
	DBProModelTrans.setOrigin(modelPosition);

	// flag to record if ragdoll still moving
	bool bStillMoving = false;

	// bool array to track which frames are part of ragdoll adjustments
	bool* pbFramePartOfBoneMovement = new bool[pObject->iFrameCount];
	for (int iF = 0; iF < pObject->iFrameCount; iF++) pbFramePartOfBoneMovement[iF] = false;

	// Loop through all bones and subtract all initial rotations of the capsules from there transforms
	for (int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		btTransform boneWorldTransform, boneRotatedWorldTransform, newLimbWorldTransform;
		boneWorldTransform.setIdentity();
		boneRotatedWorldTransform.setIdentity();
		newLimbWorldTransform.setIdentity();
		btVector3 finalWorldPosition;

		//Get the bones World Transform
		boneWorldTransform = btTransform(m_ragDollBoneArray[i]->GetRigidBody()->getWorldTransform());

		//Subtract the initial rotation of the rag doll bone
		boneRotatedWorldTransform.setBasis(boneWorldTransform.getBasis() * m_ragDollBoneArray[i]->initialRotation.inverse());

		for (int j = 0; j < m_ragDollBoneArray[i]->limbOffsets.size(); j++)
		{
			//Offsets the positions vector to ends of rag doll bones(capsules) for joint alignment.
			finalWorldPosition = boneWorldTransform * m_ragDollBoneArray[i]->limbOffsets[j];

			//Take the offset position vector and multiply by the DBpro models world transform
			finalWorldPosition = DBProModelTrans * finalWorldPosition;

			//Updated vector is new final world position for the limbs transform
			newLimbWorldTransform.setOrigin(finalWorldPosition*scaleFactor);

			//set the rotation with the bones rotated transform 
			newLimbWorldTransform.setBasis(boneRotatedWorldTransform.getBasis());

			//Handel the Models limbs rotations at a frame of animation.
			limbInitalRotationTrans.setBasis(m_ragDollBoneArray[i]->limbInitalRotation[j]);
			limbInitalRotationTrans = limbInitalRotationTrans * DBProModelTransRot.inverse();
			newLimbWorldTransform = newLimbWorldTransform * limbInitalRotationTrans;
			newLimbWorldTransform = DBProModelTransRot.inverse() * (newLimbWorldTransform * DBProModelTransRot);

			//This moves the joints but not the models bounding box that is the object
			if ( pObject->ppFrameList )
			{
				sFrame* pFrame = pObject->ppFrameList[m_ragDollBoneArray[i]->dbproLimbIDs[j]];
				if (pFrame)
				{
					pFrame->matCombined = BT2DX::BT2DX_MATRIX(newLimbWorldTransform);
					pbFramePartOfBoneMovement[m_ragDollBoneArray[i]->dbproLimbIDs[j]] = true;
				}
			}
		}

		// is this bone moving?
		float fEpsilonToFreezeLin = 0.04f;
		float fEpsilonToFreezeAng = 0.30f;
		btVector3 LinVel = m_ragDollBoneArray[i]->GetRigidBody()->getLinearVelocity();
		btVector3 AngVel = m_ragDollBoneArray[i]->GetRigidBody()->getAngularVelocity();
		if ( fabs(LinVel.getX())+fabs(LinVel.getY())+fabs(LinVel.getZ()) > fEpsilonToFreezeLin ) bStillMoving = true;
		if ( fabs(AngVel.getX())+fabs(AngVel.getY())+fabs(AngVel.getZ()) > fEpsilonToFreezeAng ) bStillMoving = true;
	}

	#ifdef WICKEDENGINE
	// go through all frame hierarchy and calc relative matrices from world matrices (and store back in matCombined)
	CalculateRelativeMatricesFromWorld(NULL, pObject->pFrame);

	for (int iF = m_iAnimateFromJoint + 1; iF < pObject->iFrameCount; iF++)
	{
		sFrame* pFrame = pObject->ppFrameList[iF];
		pFrame->matCombined = pFrame->matTransformed;
	}
	// and then give the relative matrices to wicked to animate the object
	for (int iF = m_iAnimateFromJoint; iF < pObject->iFrameCount; iF++)
	{
		if (pbFramePartOfBoneMovement[iF] == true)
		{
			sFrame* pFrame = pObject->ppFrameList[iF];
			if (pFrame)
			{
				bool bIncludeTranslation = false;
				if (iF== m_iAnimateFromJoint) bIncludeTranslation = true;
				WickedCall_OverrideLimbWithCombined(pObject, pFrame, bIncludeTranslation);
			}
		}
	}
	// must keep object playing so modified bones can affect the wicked animation system
	LoopObject(objectID, 0, 1);
	#endif

	// moved this out of wicked def as otherwise memory leak in non-MAX builds
	delete pbFramePartOfBoneMovement;
	pbFramePartOfBoneMovement = NULL;

	// if overall ragdoll has stopped moving, switch it to static
	if (bStillMoving == false) 
		SetStatic(true);
}

void DBProRagDoll::AssignLimbIDToBone(int boneIndex, int dbproLimbID)
{
	int objectID = m_id;
	m_ragDollBoneArray[boneIndex]->AddDBProLimbID(dbproLimbID);
	//Store the initial rotation of the Dbpro objects limbs.
	btMatrix3x3 matInitalRot;
	matInitalRot.setEulerYPR(btRadians(LimbDirectionZ(objectID, dbproLimbID)),
		btRadians(LimbDirectionY(objectID, dbproLimbID)),
		btRadians(LimbDirectionX(objectID, dbproLimbID)));
	m_ragDollBoneArray[boneIndex]->limbInitalRotation.push_back(matInitalRot);
}

void DBProRagDoll::HideBones()
{
	for(int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		if ( m_ragDollBoneArray[i]->GetRagDollBoneID()>0 )
			HideObject(m_ragDollBoneArray[i]->GetRagDollBoneID());
	}
}

void DBProRagDoll::ShowBones()
{
	for(int i = 0; i < m_ragDollBoneArray.size(); i++)
	{
		if ( m_ragDollBoneArray[i]->GetRagDollBoneID()>0 )
			ShowObject(m_ragDollBoneArray[i]->GetRagDollBoneID());
	}
}

void DBProRagDoll::SetDamping(btScalar linear, btScalar angular)
{
	m_linearDamping = linear;
	m_angularDamping = angular;
}

void DBProRagDoll::SetSleepingThresholds(btScalar linear, btScalar angular)
{
	m_linearSleepingThresholds = linear;
	m_angularSleepingThresholds = angular;
}

void  DBProRagDoll::SetDeactivationTime(btScalar time)
{
	m_deactivationTime = time;
}

#ifdef WICKEDENGINE
btAlignedObjectArray<DBProRagDollBone*> DBProRagDoll::GetRagdollBones()
{
	if(m_ragDollBoneArray[0] != nullptr)
	return m_ragDollBoneArray;

	btAlignedObjectArray<DBProRagDollBone*> empty;
	return empty;
}
#endif //WICKEDENGINE
