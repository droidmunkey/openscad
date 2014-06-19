#include "export.h"
#include "printutils.h"
#include "OffscreenView.h"
#include "CsgInfo.h"
#include <stdio.h>
#include "polyset.h"
#include "rendersettings.h"

#ifdef ENABLE_CGAL
#include "CGALRenderer.h"
#include "CGAL_renderer.h"
#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"

static void setupCamera(Camera &cam, const BoundingBox &bbox, float scalefactor)
{
	if (cam.type == Camera::NONE) cam.viewall = true;
	if (cam.viewall) cam.viewAll(bbox, scalefactor);
}

void export_png(const Geometry *root_geom, Camera &cam, std::ostream &output)
{
	OffscreenView *glview;
	try {
		glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
	}
	shared_ptr<const Geometry> ptr(root_geom);
	CGALRenderer cgalRenderer(ptr);

	BoundingBox bbox = cgalRenderer.getBoundingBox();
	setupCamera(cam, bbox, 3);

	glview->setCamera(cam);
	glview->setRenderer(&cgalRenderer);
	glview->setColorScheme(*cam.colorscheme);
	glview->paintGL();
	glview->save(output);
}

enum Previewer { OPENCSG, THROWNTOGETHER } previewer;

#ifdef ENABLE_OPENCSG
#include "OpenCSGRenderer.h"
#include <opencsg.h>
#endif
#include "ThrownTogetherRenderer.h"

void export_png_preview_common(Tree &tree, Camera &cam, std::ostream &output, Previewer previewer = OPENCSG)
{
	CsgInfo csgInfo = CsgInfo();
	if (!csgInfo.compile_chains(tree)) {
		fprintf(stderr,"Couldn't initialize CSG chains\n");
		return;
	}

	try {
		csgInfo.glview = new OffscreenView(cam.pixel_width, cam.pixel_height);
	} catch (int error) {
		fprintf(stderr,"Can't create OpenGL OffscreenView. Code: %i.\n", error);
		return;
	}

#ifdef ENABLE_OPENCSG
	OpenCSGRenderer openCSGRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain, csgInfo.glview->shaderinfo);
#endif
	ThrownTogetherRenderer thrownTogetherRenderer(csgInfo.root_chain, csgInfo.highlights_chain, csgInfo.background_chain);

	BoundingBox bbox;
	if (csgInfo.root_chain) bbox = csgInfo.root_chain->getBoundingBox();
	setupCamera(cam, bbox, 2.7);

	csgInfo.glview->setCamera(cam);
#ifdef ENABLE_OPENCSG
	if (previewer == OPENCSG)
		csgInfo.glview->setRenderer(&openCSGRenderer);
	else
#endif
		csgInfo.glview->setRenderer(&thrownTogetherRenderer);
#ifdef ENABLE_OPENCSG
	OpenCSG::setContext(0);
	OpenCSG::setOption(OpenCSG::OffscreenSetting, OpenCSG::FrameBufferObject);
#endif
	csgInfo.glview->setColorScheme( *cam.colorscheme );
	csgInfo.glview->paintGL();
	csgInfo.glview->save(output);
}

void export_png_with_opencsg(Tree &tree, Camera &cam, std::ostream &output)
{
#ifdef ENABLE_OPENCSG
	export_png_preview_common(tree, cam, output, OPENCSG);
#else
	fprintf(stderr,"This openscad was built without OpenCSG support\n");
#endif
}

void export_png_with_throwntogether(Tree &tree, Camera &cam, std::ostream &output)
{
	export_png_preview_common(tree, cam, output, THROWNTOGETHER);
}

#endif // ENABLE_CGAL
