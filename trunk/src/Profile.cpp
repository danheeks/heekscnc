// Profile.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "Profile.h"
#include "CNCConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "interface/HeeksObj.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyCheck.h"
#include "tinyxml/tinyxml.h"
#include "interface/Tool.h"
#include "CuttingTool.h"

#include "geometry.h"	// from the kurve directory.

#include <sstream>
#include <iomanip>

CProfileParams::CProfileParams()
{
	m_tool_on_side = 0;
	m_auto_roll_on = true;
	m_auto_roll_off = true;
	m_roll_on_point[0] = m_roll_on_point[1] = m_roll_on_point[2] = 0.0;
	m_roll_off_point[0] = m_roll_off_point[1] = m_roll_off_point[2] = 0.0;
	m_start_given = false;
	m_end_given = false;
	m_start[0] = m_start[1] = m_start[2] = 0.0;
	m_end[0] = m_end[1] = m_end[2] = 0.0;
}

void CProfileParams::set_initial_values()
{
	CNCConfig config;
	config.Read(_T("ProfileToolOnSide"), &m_tool_on_side, 1);
}

void CProfileParams::write_values_to_config()
{
	CNCConfig config;
	config.Write(_T("ProfileToolOnSide"), m_tool_on_side);
}

static void on_set_tool_on_side(int value, HeeksObj* object){
	switch(value)
	{
	case 0:
		((CProfile*)object)->m_profile_params.m_tool_on_side = 1;
		break;
	case 1:
		((CProfile*)object)->m_profile_params.m_tool_on_side = -1;
		break;
	default:
		((CProfile*)object)->m_profile_params.m_tool_on_side = 0;
		break;
	}
}
static void on_set_auto_roll_on(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_auto_roll_on = value; heeksCAD->RefreshProperties();}
static void on_set_roll_on_point(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_roll_on_point, vt, 3*sizeof(double));}
static void on_set_auto_roll_off(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_auto_roll_off = value; heeksCAD->RefreshProperties();}
static void on_set_roll_off_point(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_roll_off_point, vt, 3*sizeof(double));}
static void on_set_start_given(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_start_given = value; heeksCAD->RefreshProperties();}
static void on_set_start(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_start, vt, 3*sizeof(double));}
static void on_set_end_given(bool value, HeeksObj* object){((CProfile*)object)->m_profile_params.m_end_given = value; heeksCAD->RefreshProperties();}
static void on_set_end(const double* vt, HeeksObj* object){memcpy(((CProfile*)object)->m_profile_params.m_end, vt, 3*sizeof(double));}

void CProfileParams::GetProperties(CProfile* parent, std::list<Property *> *list)
{
	{
		std::list< wxString > choices;
		choices.push_back(_("Left"));
		choices.push_back(_("Right"));
		choices.push_back(_("On"));
		int choice = 0;
		if(m_tool_on_side == -1)choice = 1;
		else if(m_tool_on_side == 0)choice = 2;
		list->push_back(new PropertyChoice(_("tool on side"), choices, choice, parent, on_set_tool_on_side));
	}

	if(parent->m_sketches.size() == 1) // multiple sketches must use auto roll on, and can not have start and end points specified
	{
		list->push_back(new PropertyCheck(_("auto roll on"), m_auto_roll_on, parent, on_set_auto_roll_on));
		if(!m_auto_roll_on)list->push_back(new PropertyVertex(_("roll on point"), m_roll_on_point, parent, on_set_roll_on_point));
		list->push_back(new PropertyCheck(_("auto roll off"), m_auto_roll_off, parent, on_set_auto_roll_off));
		if(!m_auto_roll_off)list->push_back(new PropertyVertex(_("roll off point"), m_roll_off_point, parent, on_set_roll_off_point));
		list->push_back(new PropertyCheck(_("use start point"), m_start_given, parent, on_set_start_given));
		if(m_start_given)list->push_back(new PropertyVertex(_("start point"), m_start, parent, on_set_start));
		list->push_back(new PropertyCheck(_("use end point"), m_end_given, parent, on_set_end_given));
		if(m_end_given)list->push_back(new PropertyVertex(_("end point"), m_end, parent, on_set_end));
	}
}

void CProfileParams::WriteXMLAttributes(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "params" );
	root->LinkEndChild( element );  
	element->SetDoubleAttribute("side", m_tool_on_side);
	element->SetAttribute("auto_roll_on", m_auto_roll_on ? 1:0);
	if(!m_auto_roll_on)
	{
		element->SetDoubleAttribute("roll_onx", m_roll_on_point[0]);
		element->SetDoubleAttribute("roll_ony", m_roll_on_point[1]);
		element->SetDoubleAttribute("roll_onz", m_roll_on_point[2]);
	}
	element->SetAttribute("auto_roll_off", m_auto_roll_off ? 1:0);
	if(!m_auto_roll_off)
	{
		element->SetDoubleAttribute("roll_offx", m_roll_off_point[0]);
		element->SetDoubleAttribute("roll_offy", m_roll_off_point[1]);
		element->SetDoubleAttribute("roll_offz", m_roll_off_point[2]);
	}
	element->SetAttribute("start_given", m_start_given ? 1:0);
	if(m_start_given)
	{
		element->SetDoubleAttribute("startx", m_start[0]);
		element->SetDoubleAttribute("starty", m_start[1]);
		element->SetDoubleAttribute("startz", m_start[2]);
	}
	element->SetAttribute("end_given", m_end_given ? 1:0);
	if(m_end_given)
	{
		element->SetDoubleAttribute("endx", m_end[0]);
		element->SetDoubleAttribute("endy", m_end[1]);
		element->SetDoubleAttribute("endz", m_end[2]);
	}
}

void CProfileParams::ReadFromXMLElement(TiXmlElement* pElem)
{
	int int_for_bool;

	pElem->Attribute("side", &m_tool_on_side);
	pElem->Attribute("auto_roll_on", &int_for_bool); m_auto_roll_on = (int_for_bool != 0);
	pElem->Attribute("roll_onx", &m_roll_on_point[0]);
	pElem->Attribute("roll_ony", &m_roll_on_point[1]);
	pElem->Attribute("roll_onz", &m_roll_on_point[2]);
	pElem->Attribute("auto_roll_off", &int_for_bool); m_auto_roll_off = (int_for_bool != 0);
	pElem->Attribute("roll_offx", &m_roll_off_point[0]);
	pElem->Attribute("roll_offy", &m_roll_off_point[1]);
	pElem->Attribute("roll_offz", &m_roll_off_point[2]);
	pElem->Attribute("start_given", &int_for_bool); m_start_given = (int_for_bool != 0);
	pElem->Attribute("startx", &m_start[0]);
	pElem->Attribute("starty", &m_start[1]);
	pElem->Attribute("startz", &m_start[2]);
	pElem->Attribute("end_given", &int_for_bool); m_end_given = (int_for_bool != 0);
	pElem->Attribute("endx", &m_end[0]);
	pElem->Attribute("endy", &m_end[1]);
	pElem->Attribute("endz", &m_end[2]);
}

#define AUTO_ROLL_ON_OFF_SIZE 2.0

void CProfile::GetRollOnPos(HeeksObj* sketch, double &x, double &y)
{
	// roll on
	if(m_profile_params.m_auto_roll_on)
	{
		if(sketch)
		{
			HeeksObj* first_child = sketch->GetAtIndex(0);
			if(first_child)
			{
				double s[3];
				if(!(first_child->GetStartPoint(s)))return;
				x = s[0];
				y = s[1];
				if(m_profile_params.m_tool_on_side == 0)return;
				double v[3];
				if(heeksCAD->GetSegmentVector(first_child, 0.0, v))
				{
					double off_vec[3] = {-v[1], v[0], 0.0};
					if(m_profile_params.m_tool_on_side == -1){off_vec[0] = -off_vec[0]; off_vec[1] = -off_vec[1];}
					x = s[0] + off_vec[0] * (m_depth_op_params.m_tool_diameter/2 + AUTO_ROLL_ON_OFF_SIZE) - v[0] * AUTO_ROLL_ON_OFF_SIZE;
					y = s[1] + off_vec[1] * (m_depth_op_params.m_tool_diameter/2 + AUTO_ROLL_ON_OFF_SIZE) - v[1] * AUTO_ROLL_ON_OFF_SIZE;
				}
			}
		}
	}
	else
	{
		x = m_profile_params.m_roll_on_point[0];
		y = m_profile_params.m_roll_on_point[1];
	}
}

void CProfile::GetRollOffPos(HeeksObj* sketch, double &x, double &y)
{
	// roll off
	if(m_profile_params.m_auto_roll_off)
	{
			int num_spans = sketch->GetNumChildren();
			if(num_spans > 0)
			{
				HeeksObj* last_child = sketch->GetAtIndex(num_spans - 1);
				if(last_child)
				{
					double e[3];
					if(!(last_child->GetEndPoint(e)))return;
					x = e[0];
					y = e[1];
					if(m_profile_params.m_tool_on_side == 0)return;
					double v[3];
					if(heeksCAD->GetSegmentVector(last_child, 0.0, v))
					{
						double off_vec[3] = {-v[1], v[0], 0.0};
						if(m_profile_params.m_tool_on_side == -1){off_vec[0] = -off_vec[0]; off_vec[1] = -off_vec[1];}
						x = e[0] + off_vec[0] * (m_depth_op_params.m_tool_diameter/2 + AUTO_ROLL_ON_OFF_SIZE) + v[0] * AUTO_ROLL_ON_OFF_SIZE;
						y = e[1] + off_vec[1] * (m_depth_op_params.m_tool_diameter/2 + AUTO_ROLL_ON_OFF_SIZE) + v[1] * AUTO_ROLL_ON_OFF_SIZE;
					}
				}
			}
	}
	else
	{
		x = m_profile_params.m_roll_off_point[0];
		y = m_profile_params.m_roll_off_point[1];
	}
}


/**
	This is the duplicate of the kurve_funcs.py->make_smaller() method.  It's the one we can
	call from C++
 */
void CProfile::make_smaller( geoff_geometry::Kurve *pKurve, double *pStartx, double *pStarty, double *pFinishx, double *pFinishy ) const
{
	if (pStartx && pStarty)
	{
		int sp;
		double sx, sy, ex, ey, cx, cy;
		geoff_geometry::kurve_get_span(pKurve, 0, sp, sx, sy, ex, ey, cx, cy);

		if (pStartx) *pStartx = sx;
		if (pStarty) *pStarty = sy;

		geoff_geometry::kurve_change_start(pKurve, *pStartx, *pStarty);
	} // End if - then

	if (pFinishx && pFinishy)
	{
		int sp;
		double sx, sy, ex, ey, cx, cy;
		geoff_geometry::kurve_get_span(pKurve, 0, sp, sx, sy, ex, ey, cx, cy);

		if (pFinishx) *pFinishx = sx;
		if (pFinishy) *pFinishy = sy;

		geoff_geometry::kurve_change_start(pKurve, *pFinishx, *pFinishy);
	} // End if - then

} // End of make_smaller() method



/**
	This is the duplicate of the kurve_funcs.py->roll_on_point() method.  It's the one we can
	call from C++
 */
bool CProfile::roll_on_point( geoff_geometry::Kurve *pKurve, const wxString &direction, const double tool_radius, double *pRoll_on_x, double *pRoll_on_y) const
{
	*pRoll_on_x = double(0.0);
	*pRoll_on_y = double(0.0);

	double offset = tool_radius;
	if (direction == _T("right")) offset = -offset;
	Kurve *offset_k = geoff_geometry::kurve_new();

	bool offset_success = geoff_geometry::kurve_offset(pKurve, offset_k, offset);
	if (offset_success == false) 
	{
       		// raise "couldn't offset kurve %d" % (k)
		return(false);
	} // End if - then

	int sp;
	double sx, sy, ex, ey, cx, cy;
	double vx, vy;
	double off_vx, off_vy;

	if (geoff_geometry::kurve_num_spans(offset_k) > 0)
	{
		geoff_geometry::kurve_get_span(offset_k, 0, sp, sx, sy, ex, ey, cx, cy);
		geoff_geometry::kurve_get_span_dir(offset_k, 0, 0, vx, vy); // get start direction
		off_vx = -vy;
		off_vy = vx;

		if (direction == _T("right"))
		{
			off_vx = -off_vx;
			off_vy = -off_vy;
		} // End if - then
	
	        *pRoll_on_x = sx + off_vx * 2 - vx * 2;
        	*pRoll_on_y = sy + off_vy * 2 - vy * 2;
	} // End if - then

	return(true);

} // End roll_on_point() method



wxString CProfile::WriteSketchDefn(HeeksObj* sketch, int id_to_use, geoff_geometry::Kurve *pKurve )
{
	std::wostringstream l_ossPythonCode;

	if ((sketch->GetShortString() != NULL) && (wxString(sketch->GetShortString()).size() > 0))
	{
		l_ossPythonCode << (wxString::Format(_T("comment('%s')\n"), wxString(sketch->GetShortString()).c_str())).c_str();
	}

	l_ossPythonCode << (wxString::Format(_T("k%d = kurve.new()\n"), id_to_use > 0 ? id_to_use : sketch->m_id)).c_str();

	bool started = false;
	int sketch_id = (id_to_use > 0 ? id_to_use : sketch->m_id);

	for(HeeksObj* span_object = sketch->GetFirstChild(); span_object; span_object = sketch->GetNextChild())
	{
		double s[3] = {0, 0, 0};
		double e[3] = {0, 0, 0};
		double c[3] = {0, 0, 0};

		if(span_object){
			int type = span_object->GetType();
			if(type == LineType || type == ArcType || type == CircleType)
			{
				if(!started && type != CircleType)
				{
					span_object->GetStartPoint(s);
					l_ossPythonCode << _T("kurve.add_point(k");
					l_ossPythonCode << sketch_id;
					l_ossPythonCode << _T(", 0, ");
					l_ossPythonCode << (s[0] / theApp.m_program->m_units);
					l_ossPythonCode << _T(", ");
					l_ossPythonCode << (s[1] / theApp.m_program->m_units);
					l_ossPythonCode << _T(", 0.0, 0.0)\n");
					started = true;

					geoff_geometry::kurve_add_point(pKurve,
									0, 
									(s[0] / theApp.m_program->m_units),
									 (s[1] / theApp.m_program->m_units),
									0.0, 0.0);
				}
				span_object->GetEndPoint(e);
				if(type == LineType)
				{
					l_ossPythonCode << _T("kurve.add_point(k");
					l_ossPythonCode << sketch_id;
					l_ossPythonCode << _T(", 0, ");
					l_ossPythonCode << (e[0] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (e[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", 0.0, 0.0)\n"));

					geoff_geometry::kurve_add_point(pKurve,
									0, 
									(e[0] / theApp.m_program->m_units),
									 (e[1] / theApp.m_program->m_units),
									0.0, 0.0);
				}
				else if(type == ArcType)
				{
					span_object->GetCentrePoint(c);
					double pos[3];
					heeksCAD->GetArcAxis(span_object, pos);
					int span_type = (pos[2] >=0) ? 1:-1;
					l_ossPythonCode << (_T("kurve.add_point(k"));
					l_ossPythonCode << (sketch_id);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (span_type);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (e[0] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (e[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[0] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(")\n"));

					geoff_geometry::kurve_add_point(pKurve,
									span_type, 
									(e[0] / theApp.m_program->m_units),
									 (e[1] / theApp.m_program->m_units),
									(c[0] / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units) );
				}
				else if(type == CircleType)
				{
					span_object->GetCentrePoint(c);

					double radius = heeksCAD->CircleGetRadius(span_object);
					l_ossPythonCode << (_T("kurve.add_point(k"));
					l_ossPythonCode << (sketch_id);
					l_ossPythonCode << (_T(", 0, "));
					l_ossPythonCode << ((c[0] + radius) / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[0] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(")\n"));

					geoff_geometry::kurve_add_point(pKurve,
									0, 
									((c[0] + radius) / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units),
									(c[0] / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units) );

					l_ossPythonCode << (_T("kurve.add_point(k"));
					l_ossPythonCode << (sketch_id);
					l_ossPythonCode << (_T(", 1, "));
					l_ossPythonCode << ((c[0] - radius) / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[0] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(")\n"));

					geoff_geometry::kurve_add_point(pKurve,
									1, 
									((c[0] - radius) / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units),
									(c[0] / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units) );

					l_ossPythonCode << (_T("kurve.add_point(k"));
					l_ossPythonCode << (sketch_id);
					l_ossPythonCode << (_T(", 1, "));
					l_ossPythonCode << ((c[0] + radius) / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[0] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(", "));
					l_ossPythonCode << (c[1] / theApp.m_program->m_units);
					l_ossPythonCode << (_T(")\n"));

					geoff_geometry::kurve_add_point(pKurve,
									1, 
									((c[0] + radius) / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units),
									(c[0] / theApp.m_program->m_units),
									 (c[1] / theApp.m_program->m_units) );

				}
			}
		}
	}

	l_ossPythonCode << _T("\n");

	if(m_sketches.size() == 1 && (m_profile_params.m_start_given || m_profile_params.m_end_given))
	{
		double startx, starty, finishx, finishy;

		wxString start_string;
		if(m_profile_params.m_start_given)
		{
#ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif
			startx = m_profile_params.m_start[0] / theApp.m_program->m_units;
			starty = m_profile_params.m_start[1] / theApp.m_program->m_units;

			ss<<std::setprecision(10);
			ss << ", startx = " << startx << ", starty = " << starty;
			start_string = ss.str().c_str();
		}

		wxString finish_string;
		if(m_profile_params.m_end_given)
		{
#ifdef UNICODE
			std::wostringstream ss;
#else
			std::ostringstream ss;
#endif

			finishx = m_profile_params.m_end[0] / theApp.m_program->m_units;
			finishy = m_profile_params.m_end[1] / theApp.m_program->m_units;

			ss<<std::setprecision(10);
			ss << ", finishx = " << finishx << ", finishy = " << finishy;
			finish_string = ss.str().c_str();
		}

		l_ossPythonCode << (wxString::Format(_T("kurve_funcs.make_smaller( k%d%s%s)\n"), sketch_id, start_string.c_str(), finish_string.c_str())).c_str();
		make_smaller( 	pKurve, 
				(m_profile_params.m_start_given)?&startx:NULL,
				(m_profile_params.m_start_given)?&starty:NULL,
				(m_profile_params.m_end_given)?&finishx:NULL,
				(m_profile_params.m_end_given)?&finishy:NULL );
	}

	return(l_ossPythonCode.str().c_str());
}

wxString CProfile::AppendTextForOneSketch(HeeksObj* object, int sketch, double *pRollOnPointX, double *pRollOnPointY)
{
	std::wostringstream l_ossPythonCode;

	if(object)
	{
		geoff_geometry::Kurve *pKurve = geoff_geometry::kurve_new();
		l_ossPythonCode << WriteSketchDefn(object, sketch, pKurve).c_str();

		double total_to_cut = m_depth_op_params.m_start_depth - m_depth_op_params.m_final_depth;
		int num_step_downs = (int)(total_to_cut / fabs(m_depth_op_params.m_step_down) + 1.0 - heeksCAD->GetTolerance());

		// start - assume we are at a suitable clearance height

		// get offset side
		wxString side_string;
		switch(m_profile_params.m_tool_on_side)
		{
		case 1:
			side_string = _T("left");
			break;
		case -1:
			side_string = _T("right");
			break;
		default:
			side_string = _T("on");
			break;
		}

		// get roll on string
		wxString roll_on_string;
		if(m_profile_params.m_tool_on_side)
		{
			if(m_profile_params.m_auto_roll_on || (m_sketches.size() > 1))
			{
				l_ossPythonCode << wxString::Format(_T("roll_on_x, roll_on_y = kurve_funcs.roll_on_point(k%d, '%s', tool_diameter/2)\n"), sketch, side_string.c_str()).c_str();

				if ((pRollOnPointX != NULL) && (pRollOnPointY != NULL))
				{
					double tool_diameter = m_depth_op_params.m_tool_diameter;

					if (((COp*)this)->m_cutting_tool_number > 0)
					{
						HeeksObj *pCuttingTool = heeksCAD->GetIDObject( CuttingToolType, ((COp *)this)->m_cutting_tool_number );
						if (pCuttingTool != NULL)
						{
							tool_diameter = ((CCuttingTool *) pCuttingTool)->m_params.m_diameter;
						} // End if - then
					} // End if - then

					roll_on_point( pKurve, side_string.c_str(), tool_diameter/2, pRollOnPointX, pRollOnPointY);
				} // End if - then

				roll_on_string = wxString(_T("roll_on_x, roll_on_y"));
			}
			else
			{
#ifdef UNICODE
				std::wostringstream ss;
#else
				std::ostringstream ss;
#endif
				ss.imbue(std::locale("C"));
				ss<<std::setprecision(10);
				ss << m_profile_params.m_roll_on_point[0] / theApp.m_program->m_units << ", " << m_profile_params.m_roll_on_point[1] / theApp.m_program->m_units;
				roll_on_string = ss.str().c_str();
			}
		}
		else
		{
			l_ossPythonCode << wxString::Format(_T("sp, span1sx, span1sy, ex, ey, cx, cy = kurve.get_span(k%d, 0)\n"), sketch).c_str();
			roll_on_string = _T("span1sx, span1sy");
		}

		// rapid across to it
		l_ossPythonCode << wxString::Format(_T("rapid(%s)\n"), roll_on_string.c_str()).c_str();

		// rapid down to just above the material
		l_ossPythonCode << wxString(_T("rapid(z = rapid_down_to_height)\n")).c_str();

		wxString roll_off_string;
		if(m_profile_params.m_tool_on_side)
		{
			if(m_profile_params.m_auto_roll_off || (m_sketches.size() > 1))
			{
				l_ossPythonCode << wxString::Format(_T("roll_off_x, roll_off_y = kurve_funcs.roll_off_point(k%d, '%s', tool_diameter/2)\n"), sketch, side_string.c_str()).c_str();
				roll_off_string = wxString(_T("roll_off_x, roll_off_y"));
			}
			else
			{
#ifdef UNICODE
				std::wostringstream ss;
#else
				std::ostringstream ss;
#endif
				ss<<std::setprecision(10);
				ss << m_profile_params.m_roll_off_point[0] / theApp.m_program->m_units << ", " << m_profile_params.m_roll_off_point[1] / theApp.m_program->m_units;
				roll_off_string = ss.str().c_str();
			}
		}
		else
		{
			l_ossPythonCode << wxString::Format(_T("sp, sx, sy, ex, ey, cx, cy = kurve.get_span(k%d, kurve.num_spans(k%d) - 1)\n"), sketch, sketch).c_str();
			roll_off_string = _T("ex, ey");
		}

		if(num_step_downs > 1)
		{
			l_ossPythonCode << wxString::Format(_T("for step in range(0, %d):\n"), num_step_downs).c_str();
			l_ossPythonCode << wxString::Format(_T(" depth = start_depth + ( final_depth - start_depth ) * ( step + 1 ) / %d\n"), num_step_downs).c_str();

			// rapid across to roll on point
			l_ossPythonCode << wxString::Format(_T(" if step != 0: rapid(%s)\n"), roll_on_string.c_str()).c_str();
			// feed down to depth
			l_ossPythonCode << wxString(_T(" feed(z = depth)\n")).c_str();

			// profile the kurve
			l_ossPythonCode << wxString::Format(_T(" kurve_funcs.profile(k%d, '%s', tool_diameter/2, %s, %s)\n"), sketch, side_string.c_str(), roll_on_string.c_str(), roll_off_string.c_str()).c_str();

			// rapid back up to clearance plane
			l_ossPythonCode << wxString(_T(" rapid(z = clearance)\n")).c_str();
		}
		else
		{
			// feed down to final depth
			l_ossPythonCode << wxString(_T("feed(z = final_depth)\n")).c_str();

			// profile the kurve
			l_ossPythonCode << (wxString::Format(_T("kurve_funcs.profile(k%d, '%s', tool_diameter/2, %s, %s)\n"), sketch, side_string.c_str(), roll_on_string.c_str(), roll_off_string.c_str()).c_str());

			// rapid back up to clearance plane
			l_ossPythonCode << (wxString(_T("rapid(z = clearance)\n"))).c_str();
		}
	}

	return(l_ossPythonCode.str().c_str());
}

void CProfile::AppendTextToProgram()
{
	std::list<CDrilling::Point3d> starting_points;
	wxString python_code = AppendTextToProgram( starting_points );

	CDepthOp::AppendTextToProgram();
	theApp.m_program_canvas->m_textCtrl->AppendText( python_code.c_str() );

} // End AppendTextToProgram() method


wxString CProfile::AppendTextToProgram( std::list<CDrilling::Point3d> & starting_points )
{
	std::wostringstream l_ossPythonCode;

	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;

		// write a kurve definition
		HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
		if(object == NULL || object->GetNumChildren() == 0)continue;

		HeeksObj* re_ordered_sketch = NULL;
		SketchOrderType sketch_order = heeksCAD->GetSketchOrder(object);
		if(sketch_order == SketchOrderTypeBad)
		{
			re_ordered_sketch = object->MakeACopy();
			heeksCAD->ReOrderSketch(re_ordered_sketch, SketchOrderTypeReOrder);
			object = re_ordered_sketch;
		}

		double roll_on_point_x, roll_on_point_y;
		if(sketch_order == SketchOrderTypeMultipleCurves || sketch_order == SketchOrderHasCircles)
		{
			std::list<HeeksObj*> new_separate_sketches;
			heeksCAD->ExtractSeparateSketches(object, new_separate_sketches);
			for(std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
			{
				HeeksObj* one_curve_sketch = *It;
				l_ossPythonCode << AppendTextForOneSketch(one_curve_sketch, sketch, &roll_on_point_x, &roll_on_point_y).c_str();
				delete one_curve_sketch;

				CBox bbox;
				one_curve_sketch->GetBox(bbox);
				starting_points.push_back( CDrilling::Point3d( roll_on_point_x, roll_on_point_y, bbox.MaxZ() ) );
			}
		}
		else
		{
			l_ossPythonCode << AppendTextForOneSketch(object, sketch, &roll_on_point_x, &roll_on_point_y).c_str();
			CBox bbox;
			object->GetBox(bbox);
			starting_points.push_back( CDrilling::Point3d( roll_on_point_x, roll_on_point_y, bbox.MaxZ() ) );
		}

		if(re_ordered_sketch)
		{
			delete re_ordered_sketch;
		}
	}
	
	return( l_ossPythonCode.str().c_str() );
}

static unsigned char cross16[32] = {0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};

void CProfile::glCommands(bool select, bool marked, bool no_color)
{
	if(marked && !no_color)
	{
		// show the sketches as highlighted
		for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
		{
			int sketch = *It;
			HeeksObj* object = heeksCAD->GetIDObject(SketchType, sketch);
			if(object)object->glCommands(false, true, false);
		}

		if(m_sketches.size() == 1)
		{
			// draw roll on point
			if(!m_profile_params.m_auto_roll_on)
			{
				glColor3ub(0, 200, 200);
				glRasterPos3dv(m_profile_params.m_roll_on_point);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
			// draw roll off point
			if(!m_profile_params.m_auto_roll_on)
			{
				glColor3ub(255, 128, 0);
				glRasterPos3dv(m_profile_params.m_roll_off_point);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
			// draw start point
			if(m_profile_params.m_start_given)
			{
				glColor3ub(128, 0, 255);
				glRasterPos3dv(m_profile_params.m_start);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
			// draw end point
			if(m_profile_params.m_end_given)
			{
				glColor3ub(200, 200, 0);
				glRasterPos3dv(m_profile_params.m_end);
				glBitmap(16, 16, 8, 8, 10.0, 0.0, cross16);
			}
		}
	}
}

void CProfile::GetProperties(std::list<Property *> *list)
{
	m_profile_params.GetProperties(this, list);

	CDepthOp::GetProperties(list);
}

static CProfile* object_for_pick = NULL;

class PickStart: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick Start");}
	void Run(){if(heeksCAD->PickPosition(_("Pick new start point"), object_for_pick->m_profile_params.m_start))object_for_pick->m_profile_params.m_start_given = true;}
	wxString BitmapPath(){ return _T("pickstart");}
};

static PickStart pick_start;

class PickEnd: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pick End");}
	void Run(){if(heeksCAD->PickPosition(_("Pick new end point"), object_for_pick->m_profile_params.m_end))object_for_pick->m_profile_params.m_end_given = true;}
	wxString BitmapPath(){ return _T("pickend");}
};

static PickEnd pick_end;

void CProfile::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	object_for_pick = this;
	t_list->push_back(&pick_start);
	t_list->push_back(&pick_end);

	HeeksObj::GetTools(t_list, p);
}

HeeksObj *CProfile::MakeACopy(void)const
{
	return new CProfile(*this);
}

void CProfile::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProfile*)object));
}

bool CProfile::CanAddTo(HeeksObj* owner)
{
	return owner->GetType() == OperationsType;
}

void CProfile::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = new TiXmlElement( "Profile" );
	root->LinkEndChild( element );  
	m_profile_params.WriteXMLAttributes(element);

	// write sketch ids
	for(std::list<int>::iterator It = m_sketches.begin(); It != m_sketches.end(); It++)
	{
		int sketch = *It;
		TiXmlElement * sketch_element = new TiXmlElement( "sketch" );
		element->LinkEndChild( sketch_element );  
		sketch_element->SetAttribute("id", sketch);
	}

	WriteBaseXML(element);
}

// static member function
HeeksObj* CProfile::ReadFromXMLElement(TiXmlElement* element)
{
	CProfile* new_object = new CProfile;

	// read profile parameters
	TiXmlElement* params = TiXmlHandle(element).FirstChildElement("params").Element();
	if(params)new_object->m_profile_params.ReadFromXMLElement(params);

	// read sketch ids
	for(TiXmlElement* sketch = TiXmlHandle(element).FirstChildElement("sketch").Element(); sketch; sketch = sketch->NextSiblingElement())
	{
		int id = 0;
		sketch->Attribute("id", &id);
		if(id)new_object->m_sketches.push_back(id);
	}

	// read common parameters
	new_object->ReadBaseXML(element);

	return new_object;
}