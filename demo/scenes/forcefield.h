

class ForceField : public Scene
{
public:


	ForceField(const char* name) : Scene(name)
	{
	}

	virtual void Initialize()
	{
		const float radius = 0.01f;
		const int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);

		CreateParticleGrid(Vec3(-1.0f, radius, 0.0f), 200, 6, 50, radius*0.5f, Vec3(0.0f), 1.0f, false, 0.0f, phase);

		g_params.radius = radius*1.0f;
		g_params.dynamicFriction = 0.4f;
		g_params.staticFriction = 0.4f;
		g_params.particleFriction = 0.25f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 2;
		g_params.viscosity = 0.0f;

		g_numSubsteps = 2;

		// draw options
		g_drawPoints = true;

		callback = NULL;

	}

	virtual void PostInitialize()
	{
		// free previous callback, todo: destruction phase for tests
		if (callback)
			NvFlexExtDestroyForceFieldCallback(callback);

		// create new callback
		callback = NvFlexExtCreateForceFieldCallback(g_solver);

		// expand scene bounds to include force field
		g_sceneLower -= Vec3(1.0f);
		g_sceneUpper += Vec3(1.0f);
	}

	void DrawCircle(const Vec3& pos, const Vec3& u, const Vec3& v, float radius, int segments)
	{
		BeginLines();
		
		Vec3 start = pos + radius*v;

		for (int i=1; i <=segments; ++i)
		{
			float theta = k2Pi*(float(i)/segments);
			Vec3 end = pos + radius*sinf(theta)*u + radius*cosf(theta)*v;

			DrawLine(start, end, Vec4(1.0f));

			start = end;
		}

		EndLines();
	}

	virtual void Draw(int phase)
	{
		DrawCircle(forcefield.mPosition, Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), forcefield.mRadius, 20);
		DrawCircle(forcefield.mPosition, Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), forcefield.mRadius, 20);
	}

	virtual void Update()
	{
		float time = g_frame*g_dt;

		(Vec3&)forcefield.mPosition = Vec3((sinf(time)), 0.5f, 0.0f);
		forcefield.mRadius = (sinf(time*1.5f)*0.5f + 0.5f);
		forcefield.mStrength = -30.0f;
		forcefield.mMode = eNvFlexExtModeForce;
		forcefield.mLinearFalloff = true;

		NvFlexExtSetForceFields(callback, &forcefield, 1);
	}

	NvFlexExtForceField forcefield;

	NvFlexExtForceFieldCallback* callback;

};