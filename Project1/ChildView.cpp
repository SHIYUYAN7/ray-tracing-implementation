
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project1.h"
#include "ChildView.h"
#include "graphics/OpenGLRenderer.h"
#include "CMyRaytraceRenderer.h"
#include "graphics/GrTexture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
	m_camera.Set(20., 10., 50., 0., 0., 0., 0., 1., 0.);

	m_raytrace = false;
	m_rayimage = NULL;
	m_antialiasing = false;

	// set fog
	m_fog = false;

	CGrPtr<CGrComposite> scene = new CGrComposite;
	m_scene = scene;

	// texture
	CGrPtr<CGrTexture> sandstone = new CGrTexture;
	sandstone->LoadFile(L"textures/Chiseled_Sandstone_JE4_BE2.bmp");

	CGrPtr<CGrTexture> diamand = new CGrTexture;
	diamand->LoadFile(L"textures/Diamond_Ore_29_JE5_BE5.bmp");

	CGrPtr<CGrTexture> ice = new CGrTexture;
	ice->LoadFile(L"textures/Ice_29_JE2_BE6.bmp");


	// A red box
	CGrPtr<CGrMaterial> redpaint = new CGrMaterial;
	redpaint->AmbientAndDiffuse(0.8f, 0.0f, 0.0f);
	redpaint->Specular(0.5f, 0.5f, 0.5f);
	redpaint->SpecularOther(0.8f, 0.8f, 0.8f);
	scene->Child(redpaint);

	CGrPtr<CGrComposite> redbox = new CGrComposite;
	redpaint->Child(redbox);
	redbox->Box(-11, -11, -11, 5, 5, 5);

	// A white box
	CGrPtr<CGrMaterial> whitepaint = new CGrMaterial;
	whitepaint->AmbientAndDiffuse(0.8f, 0.8f, 0.8f);
	scene->Child(whitepaint);

	CGrPtr<CGrComposite> whitebox = new CGrComposite;
	whitepaint->Child(whitebox);
	whitebox->Box(4, 4, 4, 5, 5, 5, diamand);

	// pyramid
	CGrPtr<CGrMaterial> pyramidpaint = new CGrMaterial;
	// base color: white
	pyramidpaint->AmbientAndDiffuse(0.8f, 0.8f, 0.8f);
	scene->Child(pyramidpaint);
	
	CGrPtr<CGrComposite> pyramidconstruction = new CGrComposite;
	pyramidpaint->Child(pyramidconstruction);

	CGrPoint top = CGrPoint(0, 0, 0);
	CGrPoint a = CGrPoint(-4, -6, -4);
	CGrPoint b = CGrPoint(4, -6, -4);
	CGrPoint c = CGrPoint(4, -6, 4);
	CGrPoint d = CGrPoint(-4, -6, 4);

	// Draw the base square
	pyramidconstruction->Poly4(a, b, c, d, sandstone);

	// Draw the triangles connecting the base to the top point
	pyramidconstruction->Poly3(b, a, top, sandstone);
	pyramidconstruction->Poly3(c, b, top, sandstone);
	pyramidconstruction->Poly3(d, c, top, sandstone);
	pyramidconstruction->Poly3(a, d, top, sandstone);

	// table 
	CGrPtr<CGrMaterial> tablepaint = new CGrMaterial;
	// base color: white
	tablepaint->AmbientAndDiffuse(0.4f, 0.4f, 0.4f);
	tablepaint->SpecularOther(0.8f, 0.8f, 0.8f);
	scene->Child(tablepaint);

	CGrPtr<CGrComposite> tablesquare = new CGrComposite;
	tablepaint->Child(tablesquare);

	// Draw the base square
	tablesquare->Box(-24, -14, -24, 16, 1, 16, ice);
	tablesquare->Box(-8, -14, -24, 16, 1, 16, ice);
	tablesquare->Box(8, -14, -24, 16, 1, 16, ice);
	tablesquare->Box(-24, -14, -8, 16, 1, 16, ice);
	tablesquare->Box(-8, -14, 8, 16, 1, 16, ice);
	tablesquare->Box(8, -14, 8, 16, 1, 16, ice);
	tablesquare->Box(-24, -14, 8, 16, 1, 16, ice);
	tablesquare->Box(-8, -14, -8, 16, 1, 16, ice);
	tablesquare->Box(8, -14, -8, 16, 1, 16, ice);
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, COpenGLWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_RENDER_RAYTRACE, &CChildView::OnRenderRaytrace)
	ON_UPDATE_COMMAND_UI(ID_RENDER_RAYTRACE, &CChildView::OnUpdateRenderRaytrace)
	ON_COMMAND(ID_RENDER_FOG, &CChildView::OnRenderFog)
	ON_UPDATE_COMMAND_UI(ID_RENDER_FOG, &CChildView::OnUpdateRenderFog)
	ON_COMMAND(ID_RENDER_ADDANALITSING, &CChildView::OnRenderAddanalitsing)
	ON_UPDATE_COMMAND_UI(ID_RENDER_ADDANALITSING, &CChildView::OnUpdateRenderAddanalitsing)
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!COpenGLWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnGLDraw(CDC* pDC)
{
	if (m_raytrace)
	{
		// Clear the color buffer
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set up for parallel projection
		int width, height;
		GetSize(width, height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// If we got it, draw it
		if (m_rayimage)
		{
			glRasterPos3i(0, 0, 0);
			glDrawPixels(m_rayimagewidth, m_rayimageheight,
				GL_RGB, GL_UNSIGNED_BYTE, m_rayimage[0]);
		}

		glFlush();
	}
	else
	{
		//
		// Instantiate a renderer
		//

		COpenGLRenderer renderer;

		// Configure the renderer
		ConfigureRenderer(&renderer);

		// Enable blending and set the blending function
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//
		// Render the scene
		//

		renderer.Render(m_scene);

		// axis
		glBegin(GL_LINES);
		glVertex3d(0., 0., 0.);
		glVertex3d(12., 0., 0.);
		glVertex3d(0., 0., 0.);
		glVertex3d(0., 12., 0.);
		glVertex3d(0., 0., 0.);
		glVertex3d(0., 0., 12.);
		glEnd();
	}
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_camera.MouseDown(point.x, point.y);

	COpenGLWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_camera.MouseMove(point.x, point.y, nFlags))
		Invalidate();

	COpenGLWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_camera.MouseDown(point.x, point.y, 2);

	COpenGLWnd::OnRButtonDown(nFlags, point);
}

//
// Name :         CChildView::ConfigureRenderer()
// Description :  Configures our renderer so it is able to render the scene.
//                Indicates how we'll do our projection, where the camera is,
//                and where any lights are located.
//

void CChildView::ConfigureRenderer(CGrRenderer* p_renderer)
{
	// Determine the screen size so we can determine the aspect ratio
	int width, height;
	GetSize(width, height);
	double aspectratio = double(width) / double(height);

	//
	// Set up the camera in the renderer
	//

	p_renderer->Perspective(m_camera.FieldOfView(),
		aspectratio, // The aspect ratio.
		20., // Near clipping
		1000.); // Far clipping

	// m_camera.FieldOfView is the vertical field of view in degrees.

	//
	// Set the camera location
	//

	p_renderer->LookAt(m_camera.Eye()[0], m_camera.Eye()[1], m_camera.Eye()[2],
		m_camera.Center()[0], m_camera.Center()[1], m_camera.Center()[2],
		m_camera.Up()[0], m_camera.Up()[1], m_camera.Up()[2]);

	//
	// Set the light locations and colors
	//

	float dimd = 0.5f; // Increase this value for more intensity
	GLfloat dim[] = { dimd, dimd, dimd, 1.0f };
	GLfloat brightwhite[] = { 1.f, 1.f, 1.f, 1.0f };
	GLfloat softYellow[] = { 1.0f, 0.9f, 0.7f, 1.0f }; // Soft yellow light
	GLfloat blue[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	GLfloat red[] = { 1.0f, 0.0f, 0.0f, 1.0f };

	p_renderer->AddLight(CGrPoint(1, 0.5, 1.2, 0),
		dim, softYellow, softYellow);

	p_renderer->AddLight(CGrPoint(5, 1, 10, 0.), dim, softYellow, softYellow);

	//p_renderer->AddLight(CGrPoint(0, 1, 0, 0.),
	//	dim, brightwhite, brightwhite);

	//p_renderer->AddLight(CGrPoint(5, 1, 10, 0.), dim, brightwhite, brightwhite);
}


void CChildView::OnRenderRaytrace()
{
	m_raytrace = !m_raytrace;
	Invalidate();
	if (!m_raytrace)
		return;

	GetSize(m_rayimagewidth, m_rayimageheight);

	m_rayimage = new BYTE * [m_rayimageheight];

	int rowwid = m_rayimagewidth * 3;
	while (rowwid % 4)
		rowwid++;

	m_rayimage[0] = new BYTE[m_rayimageheight * rowwid];
	for (int i = 1; i < m_rayimageheight; i++)
	{
		m_rayimage[i] = m_rayimage[0] + i * rowwid;
	}

	for (int i = 0; i < m_rayimageheight; i++)
	{
		// Fill the image with blue
		for (int j = 0; j < m_rayimagewidth; j++)
		{
			m_rayimage[i][j * 3] = 0;               // red
			m_rayimage[i][j * 3 + 1] = 0;           // green
			m_rayimage[i][j * 3 + 2] = BYTE(255);   // blue
		}
	}
	
	// Instantiate a raytrace object
	CMyRaytraceRenderer raytrace;
	raytrace.setFog(m_fog);
	raytrace.setAntialiasing(m_antialiasing);

	// Generic configurations for all renderers
	ConfigureRenderer(&raytrace);

	//
	// Render the Scene
	//
	raytrace.SetImage(m_rayimage, m_rayimagewidth, m_rayimageheight);
	raytrace.SetWindow(this);
	raytrace.Render(m_scene);
	Invalidate();
}


void CChildView::OnUpdateRenderRaytrace(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_raytrace);
}


void CChildView::OnRenderFog()
{
	m_fog = !m_fog;
}


void CChildView::OnUpdateRenderFog(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_fog);
}


void CChildView::OnRenderAddanalitsing()
{
	m_antialiasing = !m_antialiasing;
}


void CChildView::OnUpdateRenderAddanalitsing(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_antialiasing);
}
