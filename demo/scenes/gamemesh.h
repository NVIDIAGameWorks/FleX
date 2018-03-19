
class GameMesh : public Scene
{
public:

	GameMesh(const char* name, int scene) : Scene(name), mScene(scene) {}

	void Initialize()
	{
		Mesh* level = ImportMeshFromBin(GetFilePathByPlatform("../../data/testzone.bin").c_str());		
		level->Normalize(100.0f);
		level->Transform(TranslationMatrix(Point3(0.0f, -5.0f, 0.0f)));
		level->CalculateNormals();

		Vec3 lower, upper;
		level->GetBounds(lower, upper);		
		Vec3 center = (lower+upper)*0.5f;

		NvFlexTriangleMeshId mesh = CreateTriangleMesh(level);
		AddTriangleMesh(mesh, Vec3(), Quat(), 1.0f);

		delete level;

		int group = 0;
	
		// rigids
		if (mScene == 0)
		{
			float radius = 0.05f;
			
			for (int z=0; z < 80; ++z)
				for (int x=0; x < 80; ++x)
					CreateParticleGrid(
						center - Vec3(-16.0f, 0.0f, 15.0f) + 2.0f*Vec3(x*radius*2 - 1.0f, 1.0f + radius, 1.f + z*2.0f*radius) + Vec3(Randf(radius), 0.0f, Randf(radius)), 
						2, 2 + int(Randf(0.0f, 4.0f)), 2, radius*0.9f, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(group++, 0), 0.0f);

			// separte solid particle count
			g_numSolidParticles = g_buffers->positions.size();

			g_params.radius = radius;
			g_params.dynamicFriction = 0.3f;
			g_params.dissipation = 0.0f;
			g_params.fluidRestDistance = g_params.radius*0.5f;
			g_params.viscosity = 0.05f;
			g_params.numIterations = 2;
			g_params.numPlanes = 1;
			g_params.sleepThreshold = g_params.radius*0.3f;
			g_params.maxSpeed = g_numSubsteps*g_params.radius/g_dt;
			g_params.collisionDistance = radius*0.5f;
			g_params.shapeCollisionMargin = g_params.collisionDistance*0.05f;

			g_numSubsteps = 2;
		
			g_emitters[0].mEnabled = true;	
			g_emitters[0].mSpeed = 2.0f*(g_params.radius*0.5f)/g_dt;

			// draw options		
			g_drawPoints = true;
			g_drawMesh = false;
		}

		// basic particles
		if (mScene == 1)
		{
			float radius = 0.1f;

			CreateParticleGrid(center - Vec3(2.0f, 7.0f, 2.0f) , 32, 64, 32, radius*1.02f, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), 0.0f);
		
			g_params.radius = radius;
			g_params.dynamicFriction = 0.1f;
			g_params.dissipation = 0.0f;
			g_params.numIterations = 4;
			g_params.numPlanes = 1;
			g_params.particleCollisionMargin = g_params.radius*0.1f;
			g_params.restitution = 0.0f;

			g_params.collisionDistance = g_params.radius*0.5f;
			g_params.shapeCollisionMargin = g_params.collisionDistance*0.05f;

			g_numSubsteps = 2;

			// draw options		
			g_drawPoints = true;
		}

		// fluid particles
		if (mScene == 2)
		{
			float radius = 0.1f;
			float restDistance = radius*0.6f;

			CreateParticleGrid(center - Vec3(0.0f, 7.0f, 2.0f) , 32, 64, 32, restDistance, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), 0.0f);
		
			g_params.radius = radius;
			g_params.dynamicFriction = 0.1f;
			g_params.dissipation = 0.0f;
			g_params.numPlanes = 1;
			g_params.fluidRestDistance = restDistance;
			g_params.viscosity = 0.5f;
			g_params.numIterations = 3;
			g_params.smoothing = 0.5f;
			g_params.relaxationFactor = 1.0f;
			g_params.restitution = 0.0f;
			g_params.smoothing = 0.5f;
			g_params.collisionDistance = g_params.radius*0.5f;
			g_params.shapeCollisionMargin = g_params.collisionDistance*0.05f;

			g_numSubsteps = 2;

			// draw options	
			g_drawPoints = false;
			g_drawEllipsoids = true;
			g_drawDiffuse = true;

			g_lightDistance = 5.0f;
		}
		
		// cloth
		if (mScene == 3)
		{
			float stretchStiffness = 1.0f;
			float bendStiffness = 0.8f;
			float shearStiffness = 0.8f;

			int dimx = 32;
			int dimz = 32;
			float radius = 0.05f;

			int gridx = 8;
			int gridz = 3;

			for (int x=0; x < gridx; ++x)
			{
				for (int y=0; y < 1; ++y)
				{
					for (int z=0; z < gridz; ++z)
					{
						CreateSpringGrid(center - Vec3(9.0f, 1.0f, 0.1f) + Vec3(x*dimx*radius, 0.0f, z*1.0f), dimx, dimz, 1, radius*0.95f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter), stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);
					}
				}
			}

			Vec3 l, u;
			GetParticleBounds(l, u);
			
			Vec3 center = (u+l)*0.5f;
			printf("%f %f %f\n", center.x, center.y, center.z);

			g_params.radius = radius*1.0f;
			g_params.dynamicFriction = 0.4f;
			g_params.staticFriction = 0.5f;
			g_params.dissipation = 0.0f;
			g_params.numIterations = 8;
			g_params.drag = 0.06f;
			g_params.sleepThreshold = g_params.radius*0.125f;
			g_params.relaxationFactor = 2.0f;
			g_params.collisionDistance = g_params.radius;
			g_params.shapeCollisionMargin = g_params.collisionDistance*0.05f;

			g_windStrength = 0.0f;
		
			g_numSubsteps = 2;

			// draw options		
			g_drawPoints = false;
		}
	}

	virtual void PostInitialize()
	{
		// just focus on the particles, don't need to see the whole level
		Vec3 lower, upper;
		GetParticleBounds(lower, upper);

		g_sceneUpper = upper;
		g_sceneLower = lower;
	}

	virtual void CenterCamera(void)
	{
		g_camPos = Vec3((g_sceneLower.x+g_sceneUpper.x)*0.5f, min(g_sceneUpper.y*1.25f, 6.0f), g_sceneUpper.z + min(g_sceneUpper.y, 6.0f)*2.0f);
		g_camAngle = Vec3(0.0f, -DegToRad(15.0f), 0.0f);
	}
	
	int mScene;
};
