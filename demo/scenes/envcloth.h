

class EnvironmentalCloth: public Scene
{
public:

	EnvironmentalCloth(const char* name, int dimx, int dimz, int gridx, int gridz) :
	  Scene(name), 
	  mDimX(dimx),
	  mDimZ(dimz),
	  mGridX(gridx),
	  mGridZ(gridz) {}

	virtual void Initialize()
	{	
		float scale = 1.0f;

		float minSize = 0.5f*scale;
		float maxSize = 1.0f*scale;

		for (int i=0; i < 5; i++)
			AddRandomConvex(10, Vec3(i*2.0f, 0.0f, Randf(0.0f, 2.0f)), minSize, maxSize, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi));
		
		float stretchStiffness = 0.9f;
		float bendStiffness = 0.8f;
		float shearStiffness = 0.5f;

		int dimx = mDimX;
		int dimz = mDimZ;
		float radius = 0.05f*scale;
		g_params.gravity[1] *= scale;

		int gridx = mGridX;
		int gridz = mGridZ;

		int clothIndex = 0;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);

		for (int x=0; x < gridx; ++x)
		{
			for (int y=0; y < 1; ++y)
			{
				for (int z=0; z < gridz; ++z)
				{
					clothIndex++;

					CreateSpringGrid(Vec3(x*dimx*radius, scale*(1.0f + z*0.5f), z*dimx*radius), dimx, dimz, 1, radius, phase, stretchStiffness, bendStiffness, shearStiffness, Vec3(Randf(-0.2f, 0.2f)), 1.0f);
				}
			}
		}

		g_params.radius = radius*1.05f;
		g_params.dynamicFriction = 0.25f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 8;
		g_params.viscosity = 0.0f;
		g_params.drag = 0.1f;
		g_params.lift = 0.5f;
		g_params.collisionDistance = 0.05f;
		
		// cloth converges faster with a global relaxation factor
		g_params.relaxationMode = eNvFlexRelaxationGlobal;
		g_params.relaxationFactor = 0.25f;

		g_windStrength = 0.0f;
		g_numSubsteps = 2;

		// draw options
		g_drawPoints = false;
		g_drawSprings = false;
	}
	
	int mDimX;
	int mDimZ;
	int mGridX;
	int mGridZ;
};
