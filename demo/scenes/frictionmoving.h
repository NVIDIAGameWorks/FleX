
class FrictionMovingShape: public Scene
{
public:

	FrictionMovingShape(const char* name, int type) :
	  Scene(name), mType(type) {}

	virtual void Initialize()
	{	
		float stretchStiffness = 1.0f;
		float bendStiffness = 0.5f;
		float shearStiffness = 0.5f;

		float radius = 0.05f;

		int dimx = 40;
		int dimz = 40;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);

		float spacing = radius*0.8f;

		for (int i=0; i < 3; ++i)
			CreateSpringGrid(Vec3(-dimx*spacing*0.5f, 1.5f + i*0.2f, -dimz*spacing*0.5f), dimx, dimz, 1, spacing, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);

		g_params.radius = radius*1.0f;
		g_params.dynamicFriction = 0.45f;
		g_params.particleFriction = 0.45f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 8;
		g_params.viscosity = 0.0f;
		g_params.drag = 0.05f;
		g_params.collisionDistance = radius*0.5f;
		g_params.relaxationMode = eNvFlexRelaxationGlobal;
		g_params.relaxationFactor = 0.25f;
		g_params.numPlanes = 1;

		g_numSubsteps = 2;

		g_windStrength = 0.0f;

		// draw options
		g_drawPoints = false;
		g_drawSprings = false;

		g_lightDistance *= 1.5f;

		mTime = 0.0f;

		if (mType == 3)
		{
			Mesh* m = CreateDiscMesh(0.75f, 32);
			mMesh = CreateTriangleMesh(m);
			delete m;
		}
	}

	void Update()
	{
		ClearShapes();

		mTime += g_dt;

		// let cloth settle on object
		float startTime = 1.0f;

		float time = Max(0.0f, mTime-startTime);
		float lastTime = Max(0.0f, time-g_dt);

		const float rotationSpeed = 1.0f;
		const float translationSpeed = 1.0f;

		Vec3 pos = Vec3(translationSpeed*(1.0f-cosf(time)), 0.5f, 0.0f);
		Vec3 prevPos = Vec3(translationSpeed*(1.0f-cosf(lastTime)), 0.5f, 0.0f);

		Quat rot = QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 1.0f-cosf(rotationSpeed*time));
		Quat prevRot = QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 1.0f-cosf(rotationSpeed*lastTime));

		switch(mType)
		{
		case 0:
			AddBox(Vec3(1.0f, 0.5f, 1.0f), pos, rot);
			break;
		case 1:
			AddSphere(0.5f, pos, rot);
			break;
		case 2:
			AddCapsule(0.1f, 1.5f, pos, rot);
			break;
		case 3:
			AddTriangleMesh(mMesh, pos, rot, 1.0f);
			break;
		};
		
		g_buffers->shapePrevPositions[0] = Vec4(prevPos, 0.0f);
		g_buffers->shapePrevRotations[0] = prevRot;

		UpdateShapes();
	}

	float mTime;
	int mType;

	NvFlexTriangleMeshId mMesh;
};