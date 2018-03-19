
class FluidClothCoupling : public Scene
{
public:

	FluidClothCoupling(const char* name, bool viscous) : Scene(name), mViscous(viscous) {}

	void Initialize()
	{
		float stretchStiffness = 1.0f;
		float bendStiffness = 0.4f;
		float shearStiffness = 0.4f;

		int dimx = 32;
		int dimy = 32;
		float radius = 0.1f;
		float invmass = 0.25f;
		int group = 0;

		{
			int clothStart = 0;
		
			CreateSpringGrid(Vec3(0.0f, 1.0f, 0.0f), dimx, dimy, 1, radius*0.25f, NvFlexMakePhase(group++, 0), stretchStiffness, bendStiffness, shearStiffness, Vec3(0.0f), invmass);
		
			int corner0 = clothStart + 0;
			int corner1 = clothStart + dimx-1;
			int corner2 = clothStart + dimx*(dimy-1);
			int corner3 = clothStart + dimx*dimy-1;

			g_buffers->positions[corner0].w = 0.0f;
			g_buffers->positions[corner1].w = 0.0f;
			g_buffers->positions[corner2].w = 0.0f;
			g_buffers->positions[corner3].w = 0.0f;

			// add tethers
			for (int i=clothStart; i < int(g_buffers->positions.size()); ++i)
			{
				float x = g_buffers->positions[i].x;
				g_buffers->positions[i].y = 1.5f - sinf(DegToRad(25.0f))*x;
				g_buffers->positions[i].x = cosf(DegToRad(25.0f))*x;

				//g_buffers->positions[i].y += 0.5f-g_buffers->positions[i].x;

				if (i != corner0 && i != corner1 && i != corner2 && i != corner3)
				{
					float stiffness = -0.5f;
					float give = 0.05f;

					CreateSpring(corner0, i, stiffness, give);			
					CreateSpring(corner1, i, stiffness, give);
					CreateSpring(corner2, i, stiffness, give);			
					CreateSpring(corner3, i, stiffness, give);
				}
			}

			g_buffers->positions[corner1] = g_buffers->positions[corner0] + (g_buffers->positions[corner1]-g_buffers->positions[corner0])*0.9f;
			g_buffers->positions[corner2] = g_buffers->positions[corner0] + (g_buffers->positions[corner2]-g_buffers->positions[corner0])*0.9f;
			g_buffers->positions[corner3] = g_buffers->positions[corner0] + (g_buffers->positions[corner3]-g_buffers->positions[corner0])*0.9f;
		}

		{
			// net
			int clothStart = g_buffers->positions.size();
		
			CreateSpringGrid(Vec3(0.75f, 1.0f, 0.0f), dimx, dimy, 1, radius*0.25f, NvFlexMakePhase(group++, 0), stretchStiffness, bendStiffness, shearStiffness, Vec3(0.0f), invmass);
		
			int corner0 = clothStart + 0;
			int corner1 = clothStart + dimx-1;
			int corner2 = clothStart + dimx*(dimy-1);
			int corner3 = clothStart + dimx*dimy-1;

			g_buffers->positions[corner0].w = 0.0f;
			g_buffers->positions[corner1].w = 0.0f;
			g_buffers->positions[corner2].w = 0.0f;
			g_buffers->positions[corner3].w = 0.0f;

			// add tethers
			for (int i=clothStart; i < int(g_buffers->positions.size()); ++i)
			{
				if (i != corner0 && i != corner1 && i != corner2 && i != corner3)
				{
					float stiffness = -0.5f;
					float give = 0.1f;

					CreateSpring(corner0, i, stiffness, give);			
					CreateSpring(corner1, i, stiffness, give);
					CreateSpring(corner2, i, stiffness, give);			
					CreateSpring(corner3, i, stiffness, give);
				}
			}

			g_buffers->positions[corner1] = g_buffers->positions[corner0] + (g_buffers->positions[corner1]-g_buffers->positions[corner0])*0.8f;
			g_buffers->positions[corner2] = g_buffers->positions[corner0] + (g_buffers->positions[corner2]-g_buffers->positions[corner0])*0.8f;
			g_buffers->positions[corner3] = g_buffers->positions[corner0] + (g_buffers->positions[corner3]-g_buffers->positions[corner0])*0.8f;

		}

		{
			// net
			int clothStart = g_buffers->positions.size();
		
			CreateSpringGrid(Vec3(1.5f, 0.5f, 0.0f), dimx, dimy, 1, radius*0.25f, NvFlexMakePhase(group++, 0), stretchStiffness, bendStiffness, shearStiffness, Vec3(0.0f), invmass);
		
			int corner0 = clothStart + 0;
			int corner1 = clothStart + dimx-1;
			int corner2 = clothStart + dimx*(dimy-1);
			int corner3 = clothStart + dimx*dimy-1;

			g_buffers->positions[corner0].w = 0.0f;
			g_buffers->positions[corner1].w = 0.0f;
			g_buffers->positions[corner2].w = 0.0f;
			g_buffers->positions[corner3].w = 0.0f;

			// add tethers
			for (int i=clothStart; i < int(g_buffers->positions.size()); ++i)
			{
				if (i != corner0 && i != corner1 && i != corner2 && i != corner3)
				{
					float stiffness = -0.5f;
					float give = 0.1f;

					CreateSpring(corner0, i, stiffness, give);			
					CreateSpring(corner1, i, stiffness, give);
					CreateSpring(corner2, i, stiffness, give);			
					CreateSpring(corner3, i, stiffness, give);
				}
			}

			g_buffers->positions[corner1] = g_buffers->positions[corner0] + (g_buffers->positions[corner1]-g_buffers->positions[corner0])*0.8f;
			g_buffers->positions[corner2] = g_buffers->positions[corner0] + (g_buffers->positions[corner2]-g_buffers->positions[corner0])*0.8f;
			g_buffers->positions[corner3] = g_buffers->positions[corner0] + (g_buffers->positions[corner3]-g_buffers->positions[corner0])*0.8f;

		}

		g_numSolidParticles = g_buffers->positions.size();
		g_ior = 1.0f;

		g_numExtraParticles = 64*1024;

		g_params.radius = radius;
		g_params.numIterations = 5;
		g_params.vorticityConfinement = 0.0f;
		g_params.fluidRestDistance = g_params.radius*0.5f;
		g_params.smoothing = 0.5f;
		g_params.solidPressure = 0.25f;
		g_numSubsteps = 3;		
		//g_params.numIterations = 6;

		g_params.maxSpeed = 0.5f*g_numSubsteps*g_params.radius/g_dt;

		g_maxDiffuseParticles = 32*1024;
		g_diffuseScale = 0.5f;
		g_lightDistance = 3.0f;

		// for viscous goo
		if (mViscous)
		{
			g_fluidColor = Vec4(0.0f, 0.8f, 0.2f, 1.0f);

			g_params.dynamicFriction = 0.3f;
			g_params.cohesion = 0.025f;
			g_params.viscosity = 50.85f;			
		}
		else
		{
			g_params.dynamicFriction = 0.125f;
			g_params.viscosity = 0.1f;
			g_params.cohesion = 0.0035f;
			g_params.viscosity = 4.0f;
		}

		g_emitters[0].mEnabled = false;

		Emitter e;
		e.mDir = Normalize(Vec3(1.0f, 0.0f, 0.0f));
		e.mEnabled = true;
		e.mPos = Vec3(-0.25f, 1.75f, 0.5f);
		e.mRight = Cross(e.mDir, Vec3(0.0f, 0.0f, 1.0f));
		e.mSpeed = (g_params.fluidRestDistance/(g_dt*2.0f));

		g_emitters.push_back(e);

		// draw options		
		g_drawPoints = false;
		g_drawSprings = false;
		g_drawEllipsoids = true;
	}

	bool mViscous;
};