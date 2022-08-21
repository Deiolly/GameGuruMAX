#pragma once

#include "preprocessor-flags.h"
#include <vector>
#include <DirectXMath.h>

#ifdef BUILDINGEDITOR

namespace BuildingEditor
{
	void init();
	void loop();
	void loopimgui();
	void loopinput();
	void free();
	void save();
	void optimise();
	void load();
	bool isActive();
	void createMesh(float width, float height, float depth);

	enum Settings
	{
		NONE = 0,
		ROOM = 1 << 0,
		WALL = 1 << 1,
		WINDOW = 1 << 2,
		DOOR = 1 << 3,
		FLOOR = 1 << 4,
		CEILING = 1 << 5,
		STAIRS = 1 << 6
	};

	enum class CollisionLayers
	{
		NONE = 0,
		GROUND = 1 << 0,
		WALL = 1 << 1,
		ROOF = 1 << 2,
		FLOOR = 1 << 3,
	};

	struct BuildingParameters
	{
		float cursorPos[3] = { 0.0f }; // The most up-to-date cursor position.
		int buildFrom[3] = { 0 }; // Cursor position when left-mouse is pressed.
		int buildTo[3] = { 0 }; // Cursor position when left-mouse is released.
		int currentSetting = ROOM;
		int iGridSize = 50; // will probably need to hide this from the user.
		int iWallHeight = 250;
		int iWallDepth = 10;
	};

	struct InputParameters
	{
		int mouseLeftState = 0;
		int mouseLeftPressed = 0;
		int mouseLeftReleased = 0;
		int prevMouseLeft = 0;
	};

	struct Wall
	{
		int wallDepth = 0;
		int wallHeight = 0;
		int wallWidth = 0;
		float rotationY = 0.0f;
		int position[3] = { 0 };
		int obj; // temporary.
	};

	struct Plane
	{
		DirectX::XMVECTOR normal;
		DirectX::XMVECTOR point;
		DirectX::XMVECTOR plane;
	};

	struct CSGBrush
	{
		std::vector<Plane> planes;
	};

	int TestPointToPlane(const Plane& plane, const DirectX::XMFLOAT3& point);
	bool PlanePlaneIntersect(DirectX::XMVECTOR* outPoint0, DirectX::XMVECTOR* outPoint1, const DirectX::XMVECTOR& plane0, const DirectX::XMVECTOR& plane1);
	bool LinePlaneIntersect(DirectX::XMVECTOR* outIntersectionPoint, const DirectX::XMVECTOR& plane, const DirectX::XMVECTOR& linePoint0, const DirectX::XMVECTOR& linePoint1);
	void CSGSubtract(CSGBrush& a, CSGBrush& b);
}

#endif