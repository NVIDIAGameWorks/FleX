
class ParachutingBunnies : public Scene
{
public:

	ParachutingBunnies(const char* name) : Scene(name) {}

	void Initialize()
	{
		float stretchStiffness = 1.0f;
		float bendStiffness = 0.8f;
		float shearStiffness = 0.8f;

		int dimx = 32;
		int dimy = 32;
		float radius = 0.055f;

		float height = 10.0f;
		float spacing = 1.5f;
		int numBunnies = 2;
		int group = 0;

		for (int i=0; i < numBunnies; ++i)
		{
			CreateSpringGrid(Vec3(i*dimx*radius, height + i*spacing, 0.0f), dimx, dimy, 1, radius, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide), stretchStiffness, bendStiffness, shearStiffness, Vec3(0.0f), 1.1f);

			const int startIndex = i*dimx*dimy;

			int corner0 = startIndex + 0;
			int corner1 = startIndex + dimx-1;
			int corner2 = startIndex + dimx*(dimy-1);
			int corner3 = startIndex + dimx*dimy-1;

			CreateSpring(corner0, corner1, 1.f,-0.1f);
			CreateSpring(corner1, corner3, 1.f,-0.1f);
			CreateSpring(corner3, corner2, 1.f,-0.1f);
			CreateSpring(corner0, corner2, 1.f,-0.1f);
		}

		for (int i=0; i < numBunnies; ++i)
		{		
			Vec3 velocity = RandomUnitVector()*1.0f;
			float size = radius*8.5f;

			CreateParticleShape(GetFilePathByPlatform("../../data/bunny.ply").c_str(), Vec3(i*dimx*radius + radius*0.5f*dimx - 0.5f*size, height + i*spacing-0.5f, radius*0.5f*dimy - 0.5f), size, 0.0f, radius, velocity, 0.15f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);			

			const int startIndex = i*dimx*dimy;
			const int attachIndex = g_buffers->positions.size()-1;
			g_buffers->positions[attachIndex].w = 2.0f;

			int corner0 = startIndex + 0;
			int corner1 = startIndex + dimx-1;
			int corner2 = startIndex + dimx*(dimy-1);
			int corner3 = startIndex + dimx*dimy-1;

			Vec3 attachPosition = (Vec3(g_buffers->positions[corner0]) + Vec3(g_buffers->positions[corner1]) + Vec3(g_buffers->positions[corner2]) + Vec3(g_buffers->positions[corner3]))*0.25f;
			attachPosition.y = height + i*spacing-0.5f;

			if (1)
			{
				int c[4] = {corner0, corner1, corner2, corner3};

				for (int i=0; i < 4; ++i)
				{
					Rope r;

					int start = g_buffers->positions.size();
	
					r.mIndices.push_back(attachIndex);

					Vec3 d0 = Vec3(g_buffers->positions[c[i]])-attachPosition;
					CreateRope(r, attachPosition, Normalize(d0), 1.2f, int(Length(d0)/radius*1.1f), Length(d0), NvFlexMakePhase(group++, 0), 0.0f, 0.5f, 0.0f);

					r.mIndices.push_back(c[i]);
					g_ropes.push_back(r);

					int end = g_buffers->positions.size()-1;
					

					CreateSpring(attachIndex, start, 1.2f, -0.5f);
					CreateSpring(c[i], end, 1.0f);
				}
			}
		}

		if (1)
		{
			// falling objects
			Vec3 lower, upper;
			GetParticleBounds(lower, upper);

			Vec3 center = (lower+upper)*0.5f;
			center.y = 0.0f;

			float width = (upper-lower).x*0.5f;
			float edge = 0.125f;
			float height = 0.5f;
		
			// big blocks
			for (int i=0; i < 3; ++i)
				CreateParticleShape(GetFilePathByPlatform("../../data/box.ply").c_str(), center + Vec3(float(i)-1.0f, 5.0f, 0.0f), radius*9, 0.0f, radius*0.9f, Vec3(0.0f), 0.5f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f, 0.0f, -radius*1.5f);		

			// small blocks			
			for (int j=0; j < 2; ++j)
				for (int i=0; i < 8; ++i)
					CreateParticleShape(GetFilePathByPlatform("../../data/box.ply").c_str(), Vec3(lower.x + 0.5f, 0.0f, lower.z - 0.5f) + Vec3(float(i/3), 6.0f + float(j), float(i%3)) + RandomUnitVector()*0.5f, radius*4, 0.0f, radius*0.9f, Vec3(0.0f), 1.f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f, 0.0f, -radius*2.0f);			

			g_numSolidParticles = g_buffers->positions.size();

			{
				AddBox(Vec3(edge, height, width+edge*2.0f), center + Vec3(-width - edge, height/2.0f, 0.0f));
				AddBox(Vec3(edge, height, width+edge*2.0f), center + Vec3(width + edge, height/2.0f, 0.0f));

				AddBox(Vec3(width+2.0f*edge, height, edge), center + Vec3(0.0f, height/2.0f, -(width+edge)));
				AddBox(Vec3(width+2.0f*edge, height, edge), center + Vec3(0.0f, height/2.0f, width+edge));

				float fluidWidth = width;
				float fluidHeight = height*1.25f;

				int particleWidth = int(2.0f*fluidWidth/radius);
				int particleHeight = int(fluidHeight/radius); 

				CreateParticleGrid(center - Vec3(fluidWidth, 0.0f, fluidWidth), particleWidth, particleHeight, particleWidth, radius, Vec3(0.0f), 2.0f, false, 0.0f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid));
			}
		}

		g_params.fluid = true;
		g_params.radius = 0.1f;
		g_params.fluidRestDistance = radius;
		g_params.numIterations = 4;
		g_params.viscosity = 1.0f;
		g_params.dynamicFriction = 0.05f;
		g_params.staticFriction = 0.0f;
		g_params.particleCollisionMargin = 0.0f;
		g_params.collisionDistance = g_params.fluidRestDistance*0.5f;
		g_params.vorticityConfinement = 120.0f;
		g_params.cohesion = 0.0025f;
		g_params.drag = 0.06f;
		g_params.lift = 0.f;
		g_params.solidPressure = 0.0f;
		g_params.anisotropyScale = 22.0f;
		g_params.smoothing = 1.0f;
		g_params.relaxationFactor = 1.0f;
		
		g_maxDiffuseParticles = 64*1024;
		g_diffuseScale = 0.25f;		
		g_diffuseShadow = false;
		g_diffuseColor = 2.5f;
		g_diffuseMotionScale = 1.5f;
		g_params.diffuseThreshold *= 0.01f;
		g_params.diffuseBallistic = 35;

		g_windStrength = 0.0f;
		g_windFrequency = 0.0f;

		g_numSubsteps = 2;

		// draw options		
		g_drawEllipsoids = true;
		g_drawPoints = false;
		g_drawDiffuse = true;
		g_drawSprings = 0;

		g_ropeScale = 0.2f;
		g_warmup = false;
	}
};
