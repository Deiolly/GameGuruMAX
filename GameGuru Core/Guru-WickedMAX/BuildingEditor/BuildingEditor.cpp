#include "BuildingEditor.h"
#include "master.h"
#include "gameguru.h"
#include "GGTerrain/GGTerrain.h"

#ifdef BUILDINGEDITOR

extern bool bImGuiGotFocus;

namespace BuildingEditor
{
	// Object ID's
	int iObjectOffset = 0;
	int iImageOffset = 0;
	int iObjectOffsetMax = 0;
	int iCursorObj = 0;
	int iPreviewObj = 0;

	// Settings
	int state = 0;
	BuildingParameters buildingParams;
	InputParameters inputParams;

	std::vector<Wall> walls;

	std::array<const char*, 3> modes = {"None", "Room", "Wall" };

	CSGBrush wall;
	CSGBrush window;

	void init()
	{
		iObjectOffset = g.buildingeditorobjoffset;
		iImageOffset = g.buildingeditorimgoffset;
		iObjectOffsetMax = g.buildingeditoroffsetmax;

		// Create the 'cursor' object
		WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
		iCursorObj = iObjectOffset;
		if(ObjectExist(iCursorObj) == 0)
			MakeObjectBox(iCursorObj, 5, buildingParams.iWallHeight, 5);
		WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);

		// Create the box object that shows a preview of the size of a new room.
		iPreviewObj = iObjectOffset + 1;

		state = 1;
		t.ebe.on = 1;

		// TEMP: test for csg.
		MakeObjectBox(iCursorObj + 3, 100, 100, 50);
		PositionObject(iCursorObj + 3, 0, 50, 0);
		SetObjectDiffuse(iCursorObj + 3, Rgb(255, 0, 0));
		MakeObjectBox(iCursorObj + 4, 50, 50, 100);
		PositionObject(iCursorObj + 4, 0, 50, 0);
		SetObjectDiffuse(iCursorObj + 4, Rgb(0, 255, 0));

		XMVECTOR right = XMVectorSet(1, 0, 0, 0);
		XMVECTOR back = XMVectorSet(0, 0, 1, 0);
		XMVECTOR left = XMVectorSet(-1, 0, 0, 0);
		XMVECTOR front = XMVectorSet(0, 0, -1, 0);
		XMVECTOR top = XMVectorSet(0, 1, 0, 0);
		XMVECTOR bottom = XMVectorSet(0, -1, 0, 0);

		XMFLOAT3 wallDimensions(50, 50, 25);
		XMFLOAT3 windowDimensions(25, 25, 50);

		Plane plane;

		// Wall CSG brush.
		plane.normal = right;
		plane.point = plane.normal * wallDimensions.x;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		wall.planes.push_back(plane);
		plane.normal = back;
		plane.point = plane.normal * wallDimensions.z;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		wall.planes.push_back(plane);
		plane.normal = left;
		plane.point = plane.normal * wallDimensions.x;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		wall.planes.push_back(plane);
		plane.normal = front;
		plane.point = plane.normal * wallDimensions.z;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		wall.planes.push_back(plane);
		plane.normal = top;
		plane.point = plane.normal * wallDimensions.y;
		//plane.point = XMVectorSetY(plane.point, XMVectorGetY(plane.point) + 50);
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		wall.planes.push_back(plane);
		plane.normal = bottom;
		plane.point = plane.normal * wallDimensions.y;
		//plane.point = XMVectorSetY(plane.point, XMVectorGetY(plane.point) - 50);
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		wall.planes.push_back(plane);

		// Window CSG brush.
		plane.normal = right;
		plane.point = plane.normal * windowDimensions.x;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		window.planes.push_back(plane);
		plane.normal = back;
		plane.point = plane.normal * windowDimensions.z;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		window.planes.push_back(plane);
		plane.normal = left;
		plane.point = plane.normal * windowDimensions.x;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		window.planes.push_back(plane);
		plane.normal = front;
		plane.point = plane.normal * windowDimensions.z;
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		window.planes.push_back(plane);
		plane.normal = top;
		plane.point = plane.normal * windowDimensions.y;
		//plane.point = XMVectorSetY(plane.point, XMVectorGetY(plane.point) + 50);
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		window.planes.push_back(plane);
		plane.normal = bottom;
		plane.point = plane.normal * windowDimensions.y;
		//plane.point = XMVectorSetY(plane.point, XMVectorGetY(plane.point) - 50);
		plane.plane = XMPlaneFromPointNormal(plane.point, plane.normal);
		window.planes.push_back(plane);

		
	}

	void loop()
	{
		if (state == 0 || bImGuiGotFocus)
			return;

		// Collect mouse input state.
		wiInput::MouseState mouseState = wiInput::GetMouseState();

		// Hide the cursor when orienting camera using mouse.
		if (mouseState.right_button_press)
			HideObject(iCursorObj);
		else
			ShowObject(iCursorObj);
		
		inputParams.mouseLeftState = mouseState.left_button_press;
		inputParams.mouseLeftPressed = (mouseState.left_button_press && !inputParams.prevMouseLeft) ? 1 : 0;
		inputParams.mouseLeftReleased = (!mouseState.left_button_press && inputParams.prevMouseLeft) ? 1 : 0;
		inputParams.prevMouseLeft = mouseState.left_button_press ? 1 : 0;

		// For now, raycast terrain. TODO: Custom raycasting of rooms, walls etc
		float pickX = 0, pickY = 0, pickZ = 0;
		RAY pickRay = wiRenderer::GetPickRay((long)mouseState.position.x, (long)mouseState.position.y, master.masterrenderer);

		int pickHit = 0;
		pickHit = GGTerrain::GGTerrain_RayCast(pickRay, &pickX, &pickY, &pickZ, 0, 0, 0, 0, 0);

		if (pickHit)
		{
			// Position the cursor object on the nearest grid point to the mouse.
			buildingParams.cursorPos[0] = (int(pickX / buildingParams.iGridSize)*buildingParams.iGridSize);
			buildingParams.cursorPos[1] = (int(pickY / buildingParams.iGridSize)*buildingParams.iGridSize);
			buildingParams.cursorPos[2] = (int(pickZ / buildingParams.iGridSize)*buildingParams.iGridSize);
			PositionObject(iCursorObj, buildingParams.cursorPos[0], buildingParams.cursorPos[1], buildingParams.cursorPos[2]);
		}

		// Initial click.
		if (inputParams.mouseLeftPressed)
		{
			// Find a free object slot to create a new object.
			int objID = 0;
			for (int i = iObjectOffset + 2; i <= iObjectOffsetMax; i++)
			{
				if (ObjectExist(i) == 0)
				{
					objID = i;
					break;
				}
			}

			if (objID > 0)
			{
				switch (buildingParams.currentSetting)
				{
				case ROOM:
				{
					BuildingParameters& mode = buildingParams;
					mode.buildFrom[0] = mode.cursorPos[0];
					mode.buildFrom[1] = mode.cursorPos[1];
					mode.buildFrom[2] = mode.cursorPos[2];
					mode.buildTo[0] = mode.buildFrom[0];
					mode.buildTo[1] = mode.buildFrom[1];
					mode.buildTo[2] = mode.buildTo[2];

					break;
				}
				case WALL:
				{
					BuildingParameters& mode = buildingParams;
					mode.buildFrom[0] = mode.cursorPos[0];
					mode.buildFrom[1] = mode.cursorPos[1];
					mode.buildFrom[2] = mode.cursorPos[2];
					mode.buildTo[0] = mode.buildFrom[0];
					mode.buildTo[1] = mode.buildFrom[1];
					mode.buildTo[2] = mode.buildTo[2];
					Wall newwall;
					newwall.wallHeight = buildingParams.iWallHeight;
					newwall.wallDepth = buildingParams.iWallDepth;
					walls.push_back(newwall);
					break;
				}
				case WINDOW:
					break;

				case DOOR:
					break;

				case FLOOR:
					break;

				case CEILING:
					break;

				case STAIRS:
					break;
				}
			}
		}
			

		// Mouse held down.
		if (inputParams.mouseLeftState)
		{
			switch (buildingParams.currentSetting)
			{
			case ROOM:
			{
				buildingParams.buildTo[0] = buildingParams.cursorPos[0];
				buildingParams.buildTo[1] = buildingParams.cursorPos[1];
				buildingParams.buildTo[2] = buildingParams.cursorPos[2];

				float lengthX = fabs(buildingParams.buildTo[0] - buildingParams.buildFrom[0]);
				float centerX = (buildingParams.buildTo[0] + buildingParams.buildFrom[0]) / 2.0f;
				float lengthZ = fabs(buildingParams.buildTo[2] - buildingParams.buildFrom[2]);
				float centerZ = (buildingParams.buildFrom[2] + buildingParams.buildTo[2]) / 2.0f;

				// Make a box that shows a preview of the room size.
				if (ObjectExist(iPreviewObj))
					DeleteObject(iPreviewObj);
				MakeObjectBox(iPreviewObj, lengthX, buildingParams.iWallHeight, lengthZ);
				PositionObject(iPreviewObj, centerX, 0.05f, centerZ);
				SetObjectTransparency(iPreviewObj, 1);
				SetAlphaMappingOn(iPreviewObj, 10);
				break;
			}
			case WALL:
			{
				// Recalculate the dimensions of the wall.
				Wall& wall = walls.back();

				buildingParams.buildTo[0] = buildingParams.cursorPos[0];
				buildingParams.buildTo[1] = buildingParams.cursorPos[1];
				buildingParams.buildTo[2] = buildingParams.cursorPos[2];
				wall.wallDepth = buildingParams.iWallDepth;
				wall.wallHeight = buildingParams.iWallHeight;
				float lengthX = fabs(buildingParams.buildTo[0] - buildingParams.buildFrom[0]);
				float lengthZ = fabs(buildingParams.buildTo[2] - buildingParams.buildFrom[2]);
				int length = (int)sqrtf(lengthX * lengthX + lengthZ * lengthZ) - wall.wallDepth;
				wall.wallWidth = length;
				GGVECTOR3 direction(buildingParams.buildTo[0] - buildingParams.buildFrom[0], 0, buildingParams.buildTo[2] - buildingParams.buildFrom[2]);
				
				float mag = sqrt(direction.x * direction.x + direction.z * direction.z);
				wall.rotationY = 0.0f;
				if (mag > 0)
				{
					direction.x /= mag;
					direction.z /= mag;
					
					wall.rotationY = atan2(direction.x, direction.z);
					// Convert to degrees.
					wall.rotationY *= 57.2958f;
					// Make perpendicular to match direction to wall.from
					wall.rotationY -= 90.0f;
				}
				
				// Make a box that shows a preview of the wall size.
				if (ObjectExist(iPreviewObj))
					DeleteObject(iPreviewObj);
				MakeObjectBox(iPreviewObj, wall.wallWidth, wall.wallHeight, wall.wallDepth);
				wall.position[0] = (buildingParams.buildFrom[0] + buildingParams.buildTo[0]) / 2;
				wall.position[1] = 0.05f;
				wall.position[2] = (buildingParams.buildFrom[2] + buildingParams.buildTo[2]) / 2;
				PositionObject(iPreviewObj, wall.position[0], wall.position[1], wall.position[2]);
				RotateObject(iPreviewObj, 0, wall.rotationY, 0);
				SetObjectTransparency(iPreviewObj, 1);
				SetAlphaMappingOn(iPreviewObj, 10);
				break;
			}
			case WINDOW:
				break;

			case DOOR:
				break;

			case FLOOR:
				break;

			case CEILING:
				break;

			case STAIRS:
				break;
			
			}
		}

		// Mouse released.
		if (inputParams.mouseLeftReleased)
		{
			// Hide the preview.
			if (ObjectExist(iPreviewObj))
				DeleteObject(iPreviewObj);

			switch (buildingParams.currentSetting)
			{
			case ROOM:
			{
				// Create the room.
				int obj = 0;
				for (int i = iObjectOffset+2; i <= iObjectOffsetMax; i++)
				{
					if (ObjectExist(i) == 0 && ObjectExist(i+1) == 0 && ObjectExist(i+2) == 0 && ObjectExist(i+3) == 0)
					{
						//free object slot.
						obj = i;
						break;
					}
				}

				if (obj > 0)
				{
					int lengthY = buildingParams.iWallHeight;
					int tileWidth = buildingParams.iWallDepth;
					
					int halfX = fabs(buildingParams.buildTo[0] - buildingParams.buildFrom[0]) / 2;
					int halfZ = fabs(buildingParams.buildTo[2] - buildingParams.buildFrom[2]) / 2;
					int centerX = (buildingParams.buildFrom[0] + buildingParams.buildTo[0]) / 2;
					int centerZ = (buildingParams.buildFrom[2] + buildingParams.buildTo[2]) / 2;

					int widthX = fabs(buildingParams.buildTo[0] - buildingParams.buildFrom[0]);
					int widthZ = fabs(buildingParams.buildTo[2] - buildingParams.buildFrom[2]);

					// Room composed of four walls (need to be able to delete individual walls).
					Wall wall;
					wall.wallDepth = buildingParams.iWallDepth;
					wall.wallHeight = buildingParams.iWallHeight;

					// First wall
					wall.wallWidth = widthX + wall.wallDepth;
					wall.rotationY = 0.0f;
					wall.position[0] = centerX;
					wall.position[1] = 0.05f;
					wall.position[2] = centerZ + halfZ;
					MakeObjectBox(obj, wall.wallWidth, wall.wallHeight, wall.wallDepth);
					PositionObject(obj, wall.position[0], wall.position[1], wall.position[2]);
					SetObjectDiffuse(obj, Rgb(0, 255, 0));
					wall.obj = obj;
					walls.push_back(wall);
					obj++;

					// Second
					wall.wallWidth = widthX + wall.wallDepth;
					wall.rotationY = 0.0f;
					wall.position[0] = centerX;
					wall.position[1] = 0.05f;
					wall.position[2] = centerZ - halfZ;
					MakeObjectBox(obj, wall.wallWidth, wall.wallHeight, wall.wallDepth);
					PositionObject(obj, wall.position[0], wall.position[1], wall.position[2]);
					wall.obj = obj;
					walls.push_back(wall);
					obj++;

					// Third
					wall.wallWidth = widthZ - wall.wallDepth;
					wall.rotationY = 90.0f;
					wall.position[0] = centerX - halfX;
					wall.position[1] = 0.05f;
					wall.position[2] = centerZ;
					MakeObjectBox(obj, wall.wallWidth, wall.wallHeight, wall.wallDepth);
					PositionObject(obj, wall.position[0], wall.position[1], wall.position[2]);
					RotateObject(obj, 0, wall.rotationY, 0);
					wall.obj = obj;
					walls.push_back(wall);
					obj++;

					// Fourth
					wall.wallWidth = widthZ - wall.wallDepth;
					wall.rotationY = 90.0f;
					wall.position[0] = centerX + halfX;
					wall.position[1] = 0.05f;
					wall.position[2] = centerZ;
					MakeObjectBox(obj, wall.wallWidth, wall.wallHeight, wall.wallDepth);
					PositionObject(obj, wall.position[0], wall.position[1], wall.position[2]);
					RotateObject(obj, 0, wall.rotationY, 0);
					SetObjectDiffuse(obj, Rgb(255, 0, 0));
					wall.obj = obj;
					walls.push_back(wall);
				}
				break;
			}
			case WALL:
			{
				// Create the wall.
				int obj = 0;
				for (int i = iObjectOffset + 2; i <= iObjectOffsetMax; i++)
				{
					if (ObjectExist(i) == 0)
					{
						//free object slot.
						obj = i;
						break;
					}
				}

				if (obj > 0)
				{
					Wall& wall = walls.back();
					MakeObjectBox(obj, wall.wallWidth, wall.wallHeight, wall.wallDepth);
					PositionObject(obj, wall.position[0], wall.position[1], wall.position[2]);
					RotateObject(obj, 0, wall.rotationY, 0);
					wall.obj = obj;
				}
				break;
			}

			case WINDOW:
				break;

			case DOOR:
				break;

			case FLOOR:
				break;

			case CEILING:
				break;

			case STAIRS:
				break;
			}
		}
	}

	void loopimgui()
	{
		if(state == 0)
			return;

		bool bBuildingEditorActive = isActive();

		ImGui::Begin("Building Editor##BuildingEditorSettings", &bBuildingEditorActive, 0);// ImGuiWindowFlags_None | ImGuiWindowFlags_NoMove);

		ImGui::SetNextItemOpen(true);
		if (ImGui::StyleCollapsingHeader("Settings", 0))
		{
			ImGui::Indent(10);

			ImGui::TextCenter("Grid Size");
			if (ImGui::MaxSliderInputInt("##GridSize", &buildingParams.iGridSize, 10, 250, 0))
			{
				buildingParams.iGridSize = buildingParams.iGridSize / 10 * 10;
			}
			
			bool bRecreateCursor = false;

			ImGui::TextCenter("Wall Height");
			if (ImGui::MaxSliderInputInt("##WallHeight", &buildingParams.iWallHeight, 10, 1000, 0))
			{
				buildingParams.iWallHeight = buildingParams.iWallHeight / 10 * 10;
				bRecreateCursor = true;
			}

			ImGui::TextCenter("Wall Depth");
			if (ImGui::MaxSliderInputInt("##WallDepth", &buildingParams.iWallDepth, 10, 250, 0))
			{
				buildingParams.iWallDepth = buildingParams.iWallDepth / 10 * 10;
				bRecreateCursor = true;
			}

			if (bRecreateCursor)
			{
				// Recreate the 'cursor' object with the new dimensions.
				WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
				if (ObjectExist(iCursorObj))
					DeleteObject(iCursorObj);
				MakeObjectBox(iCursorObj, buildingParams.iWallDepth, buildingParams.iWallHeight, buildingParams.iWallDepth);
				WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
			}

			ImGui::Text("Mode");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##Mode", modes[buildingParams.currentSetting], 0))
			{
				for (int i = 1; i < modes.size(); i++)
				{
					bool bSelected = i == buildingParams.currentSetting;
					if (ImGui::Selectable(modes[i], &bSelected))
					{
						buildingParams.currentSetting = i;
					}
				}
				ImGui::EndCombo();
			}

			// Display texture preset settings based on the currently selected mode - wicked material image buttons.
			switch (buildingParams.currentSetting)
			{
			case WALL:
			{
				
				break;
			}
			case ROOM:
			{
				break;
			}
			default:
				break;
			}

			if (ImGui::Button("CSG TEST", ImVec2(20, 10)))
			{
				std::vector<XMVECTOR> verts;

				for (int i = 0; i < wall.planes.size(); i++)
				{
					int intersectCount = 0;
					for (int j = 0; j < window.planes.size(); j++)
					{
						XMVECTOR p0, p1;
						if (PlanePlaneIntersect(&p0, &p1, wall.planes[i].plane, window.planes[j].plane))
						{
							intersectCount++;
							if (intersectCount >= 3)
							{
								verts.push_back(p0);
								verts.push_back(p1);
							}
						}
					}
				}

				//DeleteObject(iCursorObj + 3);
				SetObjectTransparency(iCursorObj + 3, true);
				
				
				SetObjectTransparency(iCursorObj + 4, true);
				SetAlphaMappingOn(iCursorObj + 3, 50);
				SetAlphaMappingOn(iCursorObj + 4, 50);
				//DeleteObject(iCursorObj + 4);
				for (int i = 0; i < verts.size(); i++)
				{
					bool valid = true;
					for (int j = 0; j < window.planes.size(); j++)
					{
						if (TestPointToPlane(window.planes[j], XMFLOAT3(XMVectorGetX(verts[i]), XMVectorGetY(verts[i]), XMVectorGetZ(verts[i]))) == 1)
						{
							valid = false;
							break;
						}
					}
					
					if (valid)
					{
						MakeObjectBox(iCursorObj + 5 + i, 5, 5, 5);
						PositionObject(iCursorObj + 5 + i, XMVectorGetX(verts[i]), XMVectorGetY(verts[i]) + 50, XMVectorGetZ(verts[i]));
					}
					
				}
			}

			ImGui::Indent(-10);
		}

		ImGui::End();
	}

	void free()
	{
		// Delete any images.
		for (int i = iImageOffset; i < iObjectOffset; i++)
		{
			if (ImageExist(i))
			{
				DeleteImage(i);
			}
		}

		// Delete any objects.
		for (int i = iObjectOffset; i <= iObjectOffsetMax; i++)
		{
			if (ObjectExist(i))
				DeleteObject(i);
		}

		t.ebe.on = 0;
		state = 0;
	}

	void save()
	{
		// Save data needed to be able to edit the building again in future:
			// Need data for individual walls, floors etc

		// Save a dbo file, with all of the verts being packed into one mesh, discard individual wall etc data.
	}

	void load()
	{
		// Should load the data needed to edit the building again.

		// Loading of the building for use in levels will be handled by the object library.
	}

	void optimise()
	{
		// Combine all tiled meshes into a single mesh/single mesh per material.

		// Remove co-planar verts (unless had to alter to fit door/window).
	}

	bool isActive()
	{
		return state > 0;
	}

	void createMesh(float width, float height, float depth)
	{
	
	}

	const float fEpsilon = 0.0001f;
	int TestPointToPlane(const Plane& plane, const XMFLOAT3& point)
	{
		float nX = XMVectorGetX(plane.normal);
		float nY = XMVectorGetY(plane.normal);
		float nZ = XMVectorGetZ(plane.normal);
		float x = XMVectorGetX(plane.point);
		float y = XMVectorGetY(plane.point);
		float z = XMVectorGetZ(plane.point);

		GGVECTOR3 directionToPoint = GGVECTOR3(point.x - x, point.y - y, point.z - z);

		// Distance from plane to test point:
		float distance = nX * directionToPoint.x + nY * directionToPoint.y + nZ * directionToPoint.z;

		// Point in front of the plane
		if (distance > fEpsilon)
			return 1;

		// Point behind the plane
		if (distance < -fEpsilon)
			return 0;

		// Point on the plane
		return 2;
	}

	bool PlanePlaneIntersect(XMVECTOR* outPoint0, XMVECTOR* outPoint1, const XMVECTOR& plane0, const XMVECTOR& plane1)
	{
		XMPlaneIntersectPlane(outPoint0, outPoint1, plane0, plane1);
		XMVECTOR result = XMVectorIsNaN(*outPoint0);
	
		if (XMVectorGetX(result) == 0)
		{
			result = XMVectorIsNaN(*outPoint1);
			if (XMVectorGetX(result) == 0)
			{
				// Line of intersection represented by outPoint0 to outPoint1.
				return true;
			}
		}

		// Planes are parallel and do not intersect.
		return false;
	}

	bool LinePlaneIntersect(XMVECTOR* outIntersectionPoint, const XMVECTOR& plane, const XMVECTOR& linePoint0, const XMVECTOR& linePoint1)
	{
		*outIntersectionPoint = XMPlaneIntersectLine(plane, linePoint0, linePoint1);
		XMVECTOR result = XMVectorIsNaN(*outIntersectionPoint);
		
		// No intersection between line and plane.
		if (XMVectorGetX(result) > 0)
			return false;

		// Intersection point.
		return true;
	}

	void CSGSubtract(CSGBrush& a, CSGBrush& b)
	{

	}
}

#endif