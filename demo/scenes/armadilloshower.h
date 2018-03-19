

class ArmadilloShower : public Scene
{
public:

	ArmadilloShower(const char* name, bool viscous) : Scene(name), mViscous(viscous) {}

	virtual void Initialize()
	{
		float minSize = 0.5f;
		float maxSize = 1.0f;
		
		float radius = 0.1f;

		for (int i=0; i < 5; i++)
			AddRandomConvex(10, Vec3(i*2.0f, 0.0f, Randf(0.0f, 2.0f)), minSize, maxSize, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi*10.0f));

		g_params.radius = radius;
		
		g_params.numIterations = 3;
		g_params.vorticityConfinement = 0.0f;
		g_params.fluidRestDistance = g_params.radius*0.5f;

		if (mViscous)
		{
			g_fluidColor = Vec4(0.0f, 0.8f, 0.2f, 1.0f);

			g_params.dynamicFriction = 0.5f;
			g_params.viscosity = 10.85f;
			g_params.cohesion = 0.25f;
		}
		else
		{
			g_params.dynamicFriction = 0.025f;
			g_params.viscosity = 0.05f;
		}

		g_numExtraParticles = 64*1024;

		g_diffuseScale = 1.0f;

		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/armadillo.ply").c_str(), 128);
		AddSDF(sdf, Vec3(2.0f, 0.0f, -1.0f), Quat(), 2.0f);

		Vec3 meshLower, meshUpper;
		g_mesh->GetBounds(meshLower, meshUpper);

		Emitter e1;
		e1.mDir = Vec3(-1.0f, 0.0f, 0.0f);
		e1.mRight = Vec3(0.0f, 0.0f, 1.0f);
		e1.mPos = Vec3(-1.0f, 0.15f, 0.50f) +  meshLower + Vec3(meshUpper.x, meshUpper.y*0.75f, meshUpper.z*0.5f);
		e1.mSpeed = (g_params.radius*0.5f/g_dt)*2.0f;	// 2 particle layers per-frame
		e1.mEnabled = true;

		Emitter e2;
		e2.mDir = Vec3(1.0f, 0.0f, 0.0f);
		e2.mRight = Vec3(0.0f, 0.0f, -1.0f);
		e2.mPos = Vec3(-1.0f, 0.15f, 0.50f) + meshLower + Vec3(0.0f, meshUpper.y*0.75f, meshUpper.z*0.5f);
		e2.mSpeed = (g_params.radius*0.5f/g_dt)*2.0f; // 2 particle layers per-frame
		e2.mEnabled = true;

		g_emitters.push_back(e1);
		g_emitters.push_back(e2);

		g_emit = true;

		// draw options		
		g_drawEllipsoids = true;
	}

	bool mViscous;
};