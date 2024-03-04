#pragma once
#include <stdexcept>
#include "graphics/GrRenderer.h"
#include "graphics/RayIntersection.h"

class CColor
{
public:
    CColor(double dr, double dg, double db) : r(dr), g(dg), b(db)
    {}
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    CColor operator +(const CColor& other) {
        double dr = other.r + r;
        double dg = other.g + g;
        double db = other.b + b;
        return CColor(dr, dg, db);
    }
    const double& operator[](const int i) const
    {
        switch (i)
        {
        case 0:
            return r;
        case 1:
            return g;
        case 2:
            return b;
        default:
            throw std::out_of_range("Invalid color channel index");
        }
    }
    void Set(double dr, double dg, double db) { r = dr; g = dg; b = db; }
    void Add(double dr, double dg, double db) { r += dr; g += dg; b += db; }
};

class CMyRaytraceRenderer :
	public CGrRenderer
{
private:
    int m_rayimagewidth;
    int m_rayimageheight;
    int m_sample_per_pixels;

    //fog
    bool m_rayfog;

    BYTE** m_rayimage;
    CWnd* m_window;

    CRayIntersection m_intersection;

    std::list<CGrTransform> m_mstack;
    CGrMaterial* m_material;

public:
    CMyRaytraceRenderer();
    
    void setFog(bool flag) { m_rayfog = flag; };
    void setAntialiasing(bool flag) { m_sample_per_pixels = flag ? 16 : 1; };

    void SetImage(BYTE** image, int w, int h) { m_rayimage = image; m_rayimagewidth = w;  m_rayimageheight = h; }
    void SetWindow(CWnd* p_window);

    bool RendererStart();
    bool RendererEnd();
    void RendererMaterial(CGrMaterial* p_material);

    virtual void RendererPushMatrix();
    virtual void RendererPopMatrix();
    virtual void RendererRotate(double a, double x, double y, double z);
    virtual void RendererTranslate(double x, double y, double z);
    void RendererEndPolygon();
    void RayColor(const CRay& p_ray, CColor& p_color, int p_recurse, const CRayIntersection::Object* p_ignore);
};

