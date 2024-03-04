#include "pch.h"
#include "CMyRaytraceRenderer.h"
#include "graphics/GrTexture.h"

CMyRaytraceRenderer::CMyRaytraceRenderer()
{
	m_window = NULL;
	m_rayfog = false;
	m_sample_per_pixels = 1;
}

void CMyRaytraceRenderer::SetWindow(CWnd* p_window)
{
    m_window = p_window;
}

bool CMyRaytraceRenderer::RendererStart()
{
	m_intersection.Initialize();

	m_mstack.clear();


	// We have to do all of the matrix work ourselves.
	// Set up the matrix stack.
	CGrTransform t;
	t.SetLookAt(Eye().X(), Eye().Y(), Eye().Z(),
		Center().X(), Center().Y(), Center().Z(),
		Up().X(), Up().Y(), Up().Z());


	m_mstack.push_back(t);

	m_material = NULL;

	return true;
}

void CMyRaytraceRenderer::RendererMaterial(CGrMaterial* p_material)
{
	m_material = p_material;
}

void CMyRaytraceRenderer::RendererPushMatrix()
{
	m_mstack.push_back(m_mstack.back());
}

void CMyRaytraceRenderer::RendererPopMatrix()
{
	m_mstack.pop_back();
}

void CMyRaytraceRenderer::RendererRotate(double a, double x, double y, double z)
{
	CGrTransform r;
	r.SetRotate(a, CGrPoint(x, y, z));
	m_mstack.back() *= r;
}

void CMyRaytraceRenderer::RendererTranslate(double x, double y, double z)
{
	CGrTransform r;
	r.SetTranslate(x, y, z);
	m_mstack.back() *= r;
}

//
// Name : CMyRaytraceRenderer::RendererEndPolygon()
// Description : End definition of a polygon. The superclass has
// already collected the polygon information
//

void CMyRaytraceRenderer::RendererEndPolygon()
{
    const std::list<CGrPoint>& vertices = PolyVertices();
    const std::list<CGrPoint>& normals = PolyNormals();
    const std::list<CGrPoint>& tvertices = PolyTexVertices();

    // Allocate a new polygon in the ray intersection system
    m_intersection.PolygonBegin();
    m_intersection.Material(m_material);

    if (PolyTexture())
    {
        m_intersection.Texture(PolyTexture());
    }

    std::list<CGrPoint>::const_iterator normal = normals.begin();
    std::list<CGrPoint>::const_iterator tvertex = tvertices.begin();

    for (std::list<CGrPoint>::const_iterator i = vertices.begin(); i != vertices.end(); i++)
    {
        if (normal != normals.end())
        {
            m_intersection.Normal(m_mstack.back() * *normal);
            normal++;
        }

        if (tvertex != tvertices.end())
        {
            m_intersection.TexVertex(*tvertex);
            tvertex++;
        }

        m_intersection.Vertex(m_mstack.back() * *i);
    }

    m_intersection.PolygonEnd();
}

void CMyRaytraceRenderer::RayColor(const CRay& ray, CColor& color, int p_recurse, const CRayIntersection::Object* p_ignore)
{
	double t;                                   // Will be distance to intersection
	CGrPoint intersect;                         // Will by x,y,z location of intersection
	const CRayIntersection::Object* nearest;    // Pointer to intersecting object

	//fog color
	double fogColor[3] = { 0.77, 0.77, 0.77 };

	if (m_intersection.Intersect(ray, 1e20, p_ignore, nearest, t, intersect))
	{
		// We hit something...
		// Determine information about the intersection
		CGrPoint N;
		CGrMaterial* material;
		CGrTexture* texture;
		CGrPoint texcoord;

		m_intersection.IntersectInfo(ray, nearest, t,
			N, material, texture, texcoord);

		// control the density of fog
		double fogBase = exp(-t * 0.01);
		double rfog = (1 - fogBase) * fogColor[0];
		double gfog = (1 - fogBase) * fogColor[1];
		double bfog = (1 - fogBase) * fogColor[2];

		if (material != NULL)
		{	
			// color = compute-ambient-color
			double shading[3] = {0.0, 0.0, 0.0};

			//color.r += material->Ambient(0) * lightambient;
			//color.g += material->Ambient(1) * lightambient;
			//color.b += material->Ambient(2) * lightambient;
			// For each light
			int lightNum = LightCnt();
			for (int i = 0; i < lightNum; i++) {
				CGrRenderer::Light currentLight = GetLight(i);

				for (int j = 0; j < 3; j++) {
					shading[j] += currentLight.m_ambient[j];
				}

				//Calculate the ray of shadow
				double t2;                                   // Will be distance to intersection
				CGrPoint intersect2;                         // Will by x,y,z location of intersection
				const CRayIntersection::Object* nearest2;    // Pointer to intersecting object
				CGrPoint lightDirection;
				if (currentLight.m_pos[3] == 0) {
					lightDirection = Normalize3(currentLight.m_pos);
				}
				else {
					lightDirection = Normalize3(currentLight.m_pos - intersect);
				}

				CRay shadowRay(intersect, lightDirection);
				if (!m_intersection.Intersect(shadowRay, 1e20, nearest, nearest2, t2, intersect2))
				{
					color.r += currentLight.m_diffuse[0] * material->Diffuse(0) * max(0.0, Dot3(N, lightDirection));
					color.g += currentLight.m_diffuse[1] * material->Diffuse(1) * max(0.0, Dot3(N, lightDirection));
					color.b += currentLight.m_diffuse[2] * material->Diffuse(2) * max(0.0, Dot3(N, lightDirection));
					// Calculate the spectular color
					CGrPoint half = Normalize3(lightDirection + -ray.Direction());
					double sif = pow(Dot3(N, half), material->Shininess());

					color.r += currentLight.m_specular[0] * material->Specular(0) * sif;
					color.g += currentLight.m_specular[1] * material->Specular(1) * sif;
					color.b += currentLight.m_specular[2] * material->Specular(2) * sif;
				}

			}
			//recurse 
			if (p_recurse > 1 && (material->SpecularOther(0) > 0 || material->SpecularOther(1) > 0 || material->SpecularOther(2) > 0))
			{
				CGrPoint n = Normalize3(N);
				CGrPoint v = Normalize3(-ray.Direction());
				CColor reflectionColor(0, 0, 0);
				CGrPoint R = n * 2 * Dot3(n, v) - v;
				CRay rayReflected(intersect, R);
				RayColor(rayReflected, reflectionColor, p_recurse - 1, nearest);
				color.r += material->SpecularOther(0) * reflectionColor.r;
				color.g += material->SpecularOther(1) * reflectionColor.g;
				color.b += material->SpecularOther(2) * reflectionColor.b;
			}
			double decay = 1.0 / lightNum;
			if (texture != NULL) {
				for (int j = 0; j < 3; j++)
				{
					if (shading[j] >= 1) {
						shading[j] = 1;
					}
					if (shading[j] < 0)
					{
						shading[j] = 0;
					}
				}
				int ypoint = int(texcoord.Y() * (texture->Height() - 1));
				int xpoint = int(texcoord.X() * (texture->Width() - 1));
				// times the material if it not white
				double rTexture = texture->Row(ypoint)[3 * xpoint + 0] / 255.;
				double gTexture = texture->Row(ypoint)[3 * xpoint + 1] / 255.;
				double bTexture = texture->Row(ypoint)[3 * xpoint + 2] / 255.;
				color.Add(rTexture * shading[0], gTexture * shading[1], bTexture * shading[2]);
			}
			else {
				for (int j = 0; j < 3; j++)
				{
					if (shading[j] >= 1) {
						shading[j] = 1;
					}
					if (shading[j] < 0)
					{
						shading[j] = 0;
					}
				}
				color.Add(material->Ambient(0)* shading[0], material->Ambient(1)* shading[1], material->Ambient(2)* shading[2]);
			}
			color.r *= decay;
			color.g *= decay;
			color.b *= decay;
			// add fog
			if (m_rayfog) {
				color.Set(color.r * fogBase + rfog, color.g * fogBase + gfog, color.b * fogBase + bfog);
			}
		}
	}
	else
	{
		// We hit nothing...
		// add fog
		if (m_rayfog) {
			color.Add(fogColor[0], fogColor[1], fogColor[2]);
		}
	}
}


inline double InRange(double dcolor) {
	if (dcolor < 0.0) {
		return 0.0;
	}
	else if (dcolor > 1.0) {
		return 1.0;
	}
	else {
		return dcolor;
	}
}

bool CMyRaytraceRenderer::RendererEnd()
{
	m_intersection.LoadingComplete();

	double ymin = -tan(ProjectionAngle() / 2 * GR_DTOR);
	double yhit = -ymin * 2;

	double xmin = ymin * ProjectionAspect();
	double xwid = -xmin * 2;

	double sampleSize = 1.0 / sqrt(m_sample_per_pixels);


	for (int r = 0; r < m_rayimageheight; r++)
	{
		for (int c = 0; c < m_rayimagewidth; c++)
		{
			double colorTotal[3] = { 0, 0, 0 };

			for (int sx = 0; sx < sqrt(m_sample_per_pixels); sx++)
			{
				for (int sy = 0; sy < sqrt(m_sample_per_pixels); sy++)
				{
					double x = xmin + (c + (sx + 0.5) * sampleSize) / m_rayimagewidth * xwid;
					double y = ymin + (r + (sy + 0.5) * sampleSize) / m_rayimageheight * yhit;

					// Construct a Ray
					CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1, 0)));

					CColor color(0.0, 0.0, 0.0);
					RayColor(ray, color, 100, NULL);
					colorTotal[0] += color.r;
					colorTotal[1] += color.g;
					colorTotal[2] += color.b;
				}
			}

			// Average the color values
			colorTotal[0] /= m_sample_per_pixels;
			colorTotal[1] /= m_sample_per_pixels;
			colorTotal[2] /= m_sample_per_pixels;

			colorTotal[0] = InRange(colorTotal[0]);
			colorTotal[1] = InRange(colorTotal[1]);
			colorTotal[2] = InRange(colorTotal[2]);

			m_rayimage[r][c * 3] = BYTE(colorTotal[0] * 255.);
			m_rayimage[r][c * 3 + 1] = BYTE(colorTotal[1] * 255.);
			m_rayimage[r][c * 3 + 2] = BYTE(colorTotal[2] * 255.);
		}
		if ((r % 50) == 0)
		{
			m_window->Invalidate();
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
		}
	}
	return true;
}
