
class RigidPile : public Scene
{
public:

	RigidPile(const char* name, int brickHeight) : Scene(name), mHeight(brickHeight)
	{
	}
	
	virtual void Initialize()
	{
		int sx = 2;
		int sy = mHeight;
		int sz = 2;

		Vec3 lower(0.0f, 1.5f + g_params.radius*0.25f, 0.0f);

		int dimx = 10;
		int dimy = 10;
		int dimz = 10;

		float radius = g_params.radius;

		if (1)
		{
			Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/box.ply").c_str());

			// create a basic grid
			for (int y=0; y < dimy; ++y)
				for (int z=0; z < dimz; ++z)
					for (int x=0; x < dimx; ++x)
						CreateParticleShape(
						mesh,
						(g_params.radius*0.905f)*Vec3(float(x*sx), float(y*sy), float(z*sz)) + (g_params.radius*0.1f)*Vec3(float(x),float(y),float(z)) + lower,
						g_params.radius*0.9f*Vec3(float(sx), float(sy), float(sz)), 0.0f, g_params.radius*0.9f, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(g_buffers->rigidOffsets.size()+1, 0), true, 0.002f);// 0.002f);
		

			delete mesh;

			AddPlinth();
		}
		else
		{
			// brick work
			int wdimx = 10;
			int wdimy = 10;

			int bdimx = 4;
			int bdimy = 2;
			int bdimz = 2;

			for (int y=0; y < wdimy; ++y)
			{
				for (int x=0; x < wdimx; ++x)
				{
					Vec3 lower = Vec3(x*bdimx*radius + 0.5f*radius, 0.93f*bdimy*y*radius, 0.0f);

					if (y&1)
						lower += Vec3(bdimx*0.25f*radius + 0.5f*radius, 0.0f, 0.0f);

					//CreateParticleGrid(lower, bdimx, bdimy, bdimz, radius, Vec3(0.0f), 1.0f, true, g_buffers->rigidOffsets.size()+1, 0.0f);
					CreateParticleShape(GetFilePathByPlatform("../../data/box.ply").c_str(), lower + RandomUnitVector()*Vec3(0.0f, 0.0f, 0.0f), Vec3(bdimx*radius, bdimy*radius, bdimz*radius), 0.0f, radius, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(g_buffers->rigidOffsets.size()+1, 0), true, 0.0f, Vec3(0.0f, 0.0f, y*0.0001f));
				}
			}
			
			if (0)
			{
				// create a basic grid
				for (int y=0; y < dimy; ++y)
					for (int z=0; z < 1; ++z)
						for (int x=0; x < 1; ++x)
							CreateParticleShape(
							GetFilePathByPlatform("../../data/box.ply").c_str(), 
							0.99f*(g_params.radius)*Vec3(float(x*sx), float(y*sy), float(z*sz)),
							g_params.radius*Vec3(float(sx), float(sy), float(sz)), 0.0f, g_params.radius, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(g_buffers->rigidOffsets.size()+1, 0), true, 0.0f);

				// create a basic grid
				for (int y=0; y < dimy; ++y)
					for (int z=0; z < 1; ++z)
						for (int x=0; x < 1; ++x)
							CreateParticleShape(
							GetFilePathByPlatform("../../data/box.ply").c_str(), 
							0.99f*(g_params.radius)*Vec3(float(sx*2 + x*sx), float(y*sy), float(z*sz)),
							g_params.radius*Vec3(float(sx), float(sy), float(sz)), 0.0f, g_params.radius, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(g_buffers->rigidOffsets.size()+1, 0), true, 0.0f);
			}

		}
		

		if (0)
		{
			float stretchStiffness = 1.0f;
			float bendStiffness = 0.5f;
			float shearStiffness = 0.7f;

			int dimx = 40;
			int dimy = 40;

			CreateSpringGrid(Vec3(-1.0f, 1.0f + g_params.radius*0.5f, -1.0f), dimx, dimy, 1, g_params.radius*0.9f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), stretchStiffness, bendStiffness, shearStiffness, Vec3(0.0f), 1.0f);
		}


		//g_numExtraParticles = 32*1024;		
		g_numSubsteps = 2;
		g_params.numIterations = 8;

		g_params.radius *= 1.0f;
		g_params.dynamicFriction = 0.4f;
		g_params.dissipation = 0.01f;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.sleepThreshold = g_params.radius*0.25f;
		g_params.shockPropagation = 3.f;
		
		g_windStrength = 0.0f;

		// draw options
		g_drawPoints = false;

		g_emitters[0].mEnabled = true;
		g_emitters[0].mSpeed = (g_params.radius*2.0f/g_dt);
	}

	virtual void Update()
	{
			
	}

	int mHeight;
};
