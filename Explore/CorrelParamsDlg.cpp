/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/foreach.hpp>
#include <wx/textdlg.h>
#include <wx/valnum.h>
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../GdaConst.h"
#include "../GenGeomAlgs.h"
#include "../logger.h"
#include "../Project.h"
#include "../DialogTools/WebViewHelpWin.h"
#include "CorrelParamsDlg.h"

BEGIN_EVENT_TABLE( CorrelParamsFrame, wxDialog )
EVT_CLOSE( CorrelParamsFrame::OnClose )
END_EVENT_TABLE()

CorrelParamsFrame::CorrelParamsFrame(Project* project_)
: wxDialog(NULL, wxID_ANY, "Correlogram Parameters" , wxDefaultPosition, wxDefaultSize),
project(project_),
var_txt(0), var_choice(0), dist_txt(0), dist_choice(0), bins_txt(0),
bins_spn_ctrl(0), thresh_cbx(0), thresh_tctrl(0), thresh_slider(0),
all_pairs_rad(0), est_pairs_txt(0), est_pairs_num_txt(0),
rand_samp_rad(0), max_iter_txt(0), max_iter_tctrl(0),
help_btn(0), apply_btn(0)
{
	LOG_MSG("Entering CorrelParamsFrame::CorrelParamsFrame");
	
    {
        std::vector<wxString> tm_strs;
        project->GetTableInt()->GetTimeStrings(tm_strs);
        var_man.ClearAndInit(tm_strs);
        
        correl_params.dist_metric = project->GetDefaultDistMetric();
        correl_params.dist_units = project->GetDefaultDistUnits();

    }
    
	wxPanel* panel = new wxPanel(this);
	panel->SetBackgroundColour(*wxWHITE);
	SetBackgroundColour(*wxWHITE);
	{
		var_txt = new wxStaticText(panel, XRCID("ID_VAR_TXT"), "Variable:");
		var_choice = new wxChoice(panel, XRCID("ID_VAR_CHOICE"), wxDefaultPosition,wxSize(160,-1));
		wxString var_nm = "";
		if (var_man.GetVarsCount() > 0)
            var_nm = var_man.GetName(0);
		UpdateVarChoiceFromTable(var_nm);
	}
	wxBoxSizer* var_h_szr = new wxBoxSizer(wxHORIZONTAL);
	var_h_szr->Add(var_txt, 0, wxALIGN_CENTER_VERTICAL);
	var_h_szr->AddSpacer(5);
	var_h_szr->Add(var_choice, 0, wxALIGN_CENTER_VERTICAL);
	
	dist_txt = new wxStaticText(panel, XRCID("ID_DIST_TXT"), "Distance:");
	dist_choice = new wxChoice(panel, XRCID("ID_DIST_CHOICE"), wxDefaultPosition, wxSize(160,-1));
	dist_choice->Append("Euclidean Distance");
	dist_choice->Append("Arc Distance (mi)");
	dist_choice->Append("Arc Distance (km)");
	if (correl_params.dist_metric == WeightsMetaInfo::DM_arc) {
		if (correl_params.dist_units == WeightsMetaInfo::DU_km) {
			dist_choice->SetSelection(2);
		} else {
			dist_choice->SetSelection(1);
		}
	} else {
		dist_choice->SetSelection(0);
	}
	Connect(XRCID("ID_DIST_CHOICE"), wxEVT_CHOICE, wxCommandEventHandler(CorrelParamsFrame::OnDistanceChoiceSelected));
	wxBoxSizer* dist_h_szr = new wxBoxSizer(wxHORIZONTAL);
	dist_h_szr->Add(dist_txt, 0, wxALIGN_CENTER_VERTICAL);
	dist_h_szr->AddSpacer(5);
	dist_h_szr->Add(dist_choice, 0, wxALIGN_CENTER_VERTICAL);
	
	{
		bins_txt = new wxStaticText(panel, XRCID("ID_BINS_TXT"), "Number Bins:");
		wxString vs;
		vs << correl_params.bins;
		bins_spn_ctrl = new wxSpinCtrl(panel, XRCID("ID_BINS_SPN_CTRL"), vs,  wxDefaultPosition, wxSize(75,-1),  wxSP_ARROW_KEYS,  CorrelParams::min_bins_cnst, CorrelParams::max_bins_cnst, correl_params.bins);
		Connect(XRCID("ID_BINS_SPN_CTRL"), wxEVT_SPINCTRL, wxSpinEventHandler(CorrelParamsFrame::OnBinsSpinEvent));
		Connect(XRCID("ID_BINS_SPN_CTRL"), wxEVT_TEXT, wxCommandEventHandler(CorrelParamsFrame::OnBinsTextCtrl));
	}
	wxBoxSizer* bins_h_szr = new wxBoxSizer(wxHORIZONTAL);
	bins_h_szr->Add(bins_txt, 0, wxALIGN_CENTER_VERTICAL);
	bins_h_szr->AddSpacer(5);
	bins_h_szr->Add(bins_spn_ctrl, 0, wxALIGN_CENTER_VERTICAL);
	
	thresh_cbx = new wxCheckBox(panel, XRCID("ID_THRESH_CBX"), "Max Distance:");
	thresh_cbx->SetValue(false);
	thresh_tctrl = new wxTextCtrl(panel, XRCID("ID_THRESH_TCTRL"), "", wxDefaultPosition, wxSize(100,-1));
	thresh_tctrl->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	thresh_tctrl->Enable(false);
	//UpdateThreshTctrlVal();
	Connect(XRCID("ID_THRESH_CBX"), wxEVT_CHECKBOX,
					wxCommandEventHandler(CorrelParamsFrame::OnThreshCheckBox));
	Connect(XRCID("ID_THRESH_TCTRL"), wxEVT_TEXT,
					wxCommandEventHandler(CorrelParamsFrame::OnThreshTextCtrl));
	//Connect(XRCID("ID_THRESH_TCTRL"), wxEVT_KILL_FOCUS,
	//				wxFocusEventHandler(CorrelParamsFrame::OnThreshTctrlKillFocus));
	thresh_tctrl->Bind(wxEVT_KILL_FOCUS,
										 &CorrelParamsFrame::OnThreshTctrlKillFocus,
										 this);
	wxBoxSizer* thresh_h_szr = new wxBoxSizer(wxHORIZONTAL);
	thresh_h_szr->Add(thresh_cbx, 0, wxALIGN_CENTER_VERTICAL);
	thresh_h_szr->AddSpacer(5);
	thresh_h_szr->Add(thresh_tctrl, 0, wxALIGN_CENTER_VERTICAL);
	thresh_slider = new wxSlider(panel, XRCID("ID_THRESH_SLDR"),
															 sldr_tcks/2, 0, sldr_tcks,
															 wxDefaultPosition, wxSize(180,-1));
	Connect(XRCID("ID_THRESH_SLDR"), wxEVT_SLIDER,
					wxCommandEventHandler(CorrelParamsFrame::OnThreshSlider));
	thresh_slider->Enable(false);
	wxBoxSizer* thresh_sld_h_szr = new wxBoxSizer(wxHORIZONTAL);
	thresh_sld_h_szr->Add(thresh_slider, 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* thresh_v_szr = new wxBoxSizer(wxVERTICAL);
	thresh_v_szr->Add(thresh_h_szr, 0, wxBOTTOM, 5);
	thresh_v_szr->Add(thresh_sld_h_szr, 0, wxALIGN_CENTER_HORIZONTAL);
	
	all_pairs_rad = new wxRadioButton(panel, XRCID("ID_ALL_PAIRS_RAD"), "All Pairs", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL | wxRB_GROUP);
	all_pairs_rad->SetValue(correl_params.method == CorrelParams::ALL_PAIRS);
	Connect(XRCID("ID_ALL_PAIRS_RAD"), wxEVT_RADIOBUTTON, wxCommandEventHandler(CorrelParamsFrame::OnAllPairsRadioSelected));
    
	est_pairs_txt = new wxStaticText(panel, XRCID("ID_EST_PAIRS_TXT"), "Estimated Pairs:");
	est_pairs_num_txt = new wxStaticText(panel, XRCID("ID_EST_PAIRS_NUM_TXT"), "4,000,000");
	wxBoxSizer* est_pairs_h_szr = new wxBoxSizer(wxHORIZONTAL);
	est_pairs_h_szr->Add(est_pairs_txt, 0, wxALIGN_CENTER_VERTICAL);
	est_pairs_h_szr->AddSpacer(5);
	est_pairs_h_szr->Add(est_pairs_num_txt, 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* all_pairs_v_szr = new wxBoxSizer(wxVERTICAL);
	all_pairs_v_szr->Add(all_pairs_rad);
	all_pairs_v_szr->AddSpacer(2);
	all_pairs_v_szr->Add(est_pairs_h_szr, 0, wxLEFT, 18);
	
	rand_samp_rad = new wxRadioButton(panel, XRCID("ID_RAND_SAMP_RAD"), "Random Sample", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL);
	rand_samp_rad->SetValue(correl_params.method != CorrelParams::ALL_PAIRS);
	Connect(XRCID("ID_RAND_SAMP_RAD"), wxEVT_RADIOBUTTON, wxCommandEventHandler(CorrelParamsFrame::OnRandSampRadioSelected));
	max_iter_txt = new wxStaticText(panel, XRCID("ID_MAX_ITER_TXT"), "Iterations:");
	{
		wxString vs;
		vs << correl_params.max_iterations;
		max_iter_tctrl = new wxTextCtrl(panel, XRCID("ID_MAX_ITER_TCTRL"), vs, wxDefaultPosition, wxSize(100,-1));
		max_iter_tctrl->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
		Connect(XRCID("ID_MAX_ITER_TCTRL"), wxEVT_TEXT, wxCommandEventHandler(CorrelParamsFrame::OnMaxIterTextCtrl));
        max_iter_tctrl->Bind(wxEVT_KILL_FOCUS, &CorrelParamsFrame::OnMaxIterTctrlKillFocus, this);
	}
	wxBoxSizer* max_iter_h_szr = new wxBoxSizer(wxHORIZONTAL);
	max_iter_h_szr->Add(max_iter_txt, 0, wxALIGN_CENTER_VERTICAL);
	max_iter_h_szr->AddSpacer(8);
	max_iter_h_szr->Add(max_iter_tctrl, 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* rand_samp_v_szr = new wxBoxSizer(wxVERTICAL);
	rand_samp_v_szr->Add(rand_samp_rad);
	rand_samp_v_szr->AddSpacer(2);
	rand_samp_v_szr->Add(max_iter_h_szr, 0, wxLEFT, 18);
		
	help_btn = new wxButton(panel, XRCID("ID_HELP_BTN"), "Help", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	apply_btn = new wxButton(panel, XRCID("ID_APPLY_BTN"), "Apply", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	Connect(XRCID("ID_HELP_BTN"), wxEVT_BUTTON, wxCommandEventHandler(CorrelParamsFrame::OnHelpBtn));
	Connect(XRCID("ID_APPLY_BTN"), wxEVT_BUTTON, wxCommandEventHandler(CorrelParamsFrame::OnApplyBtn));
	wxBoxSizer* btns_h_szr = new wxBoxSizer(wxHORIZONTAL);
	btns_h_szr->Add(help_btn, 0, wxALIGN_CENTER_VERTICAL);
	btns_h_szr->AddSpacer(15);
	btns_h_szr->Add(apply_btn, 0, wxALIGN_CENTER_VERTICAL);
	
	UpdateEstPairs();
	
	// Arrange above widgets in panel using sizers.
	// Top level panel sizer will be panel_h_szr
	// Below that will be panel_v_szr
	// panel_v_szr will directly receive widgets
	
	wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
	panel_v_szr->Add(var_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->AddSpacer(2);
	panel_v_szr->Add(dist_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->AddSpacer(3);
	panel_v_szr->Add(bins_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->AddSpacer(3);
	panel_v_szr->Add(thresh_v_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->Add(all_pairs_v_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->AddSpacer(5);
	panel_v_szr->Add(rand_samp_v_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	panel_v_szr->AddSpacer(10);
	panel_v_szr->Add(btns_h_szr, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	
	wxBoxSizer* panel_h_szr = new wxBoxSizer(wxHORIZONTAL);
	panel_h_szr->Add(panel_v_szr, 1, wxEXPAND);
	panel->SetSizer(panel_h_szr);
	
	// Top Sizer for Frame
	wxBoxSizer* top_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_h_sizer->Add(panel, 1, wxEXPAND|wxALL, 8);
	
	SetSizerAndFit(top_h_sizer);
	
	wxCommandEvent ev;
    if (correl_params.method == CorrelParams::ALL_PAIRS)
        OnAllPairsRadioSelected(ev);
    else
        OnRandSampRadioSelected(ev);
	
    Center();
    
	LOG_MSG("Exiting CorrelParamsFrame::CorrelParamsFrame");
}

CorrelParamsFrame::~CorrelParamsFrame()
{
	LOG_MSG("In CorrelParamsFrame::~CorrelParamsFrame");
}

void CorrelParamsFrame::OnClose(wxCloseEvent& ev)
{
    EndModal(wxID_CANCEL);
    Destroy();
    ev.Skip();
}

void CorrelParamsFrame::OnHelpBtn(wxCommandEvent& ev)
{
	LOG_MSG("In CorrelParamsFrame::OnHelpBtn");
	WebViewHelpWin* win = new WebViewHelpWin(project, GetHelpPageHtml(), NULL,  wxID_ANY,  "Correlogram Parameters Help");
}

void CorrelParamsFrame::OnApplyBtn(wxCommandEvent& ev)
{
	LOG_MSG("In CorrelParamsFrame::OnApplyBtn");
	{
		long new_bins = bins_spn_ctrl->GetValue();
		if (new_bins < CorrelParams::min_bins_cnst) {
			new_bins = CorrelParams::min_bins_cnst;
		}
		if (new_bins > CorrelParams::max_bins_cnst) {
			new_bins = CorrelParams::max_bins_cnst;
		}
		correl_params.bins = new_bins;
	}
	{
		wxString s = dist_choice->GetStringSelection();
		if (s == "Euclidean Distance") {
			correl_params.dist_metric = WeightsMetaInfo::DM_euclidean;
		} else if (s == "Arc Distance (mi)") {
			correl_params.dist_metric = WeightsMetaInfo::DM_arc;
			correl_params.dist_units = WeightsMetaInfo::DU_mile;
		} else if (s == "Arc Distance (km)") {
			correl_params.dist_metric = WeightsMetaInfo::DM_arc;
			correl_params.dist_units = WeightsMetaInfo::DU_km;
		}
	}
	
	bool use_thresh = thresh_cbx->GetValue();
    double thresh_val = 0;
    if (use_thresh) {
        thresh_val = GetThreshMax();

		wxString val = thresh_tctrl->GetValue();
		val.Trim(false);
		val.Trim(true);
		double t;
		bool is_valid = val.ToDouble(&t);
		if (is_valid) {
			thresh_val = t;
		} else {
			use_thresh = false;
		}
	}


	if (all_pairs_rad->GetValue() == true) {
		correl_params.method = CorrelParams::ALL_PAIRS;
		if (use_thresh) {
			correl_params.method = CorrelParams::ALL_PAIRS_THRESH;
			correl_params.threshold = thresh_val;
		}
	} else {
		correl_params.method = CorrelParams::RAND_SAMP;
		if (use_thresh) {
			correl_params.method = CorrelParams::RAND_SAMP_THRESH;
			correl_params.threshold = thresh_val;
		}
		wxString s = max_iter_tctrl->GetValue();
		long v;
		long revert_val = correl_params.max_iterations;
		bool apply_revert = true;
		if (s.ToLong(&v)) {
			if (v > CorrelParams::max_iter_cnst) {
				revert_val = CorrelParams::max_iter_cnst;
				correl_params.max_iterations = CorrelParams::max_iter_cnst;
			} else {
				correl_params.max_iterations = v;
				apply_revert = false;
			}
		} 
		if (apply_revert) {
			wxString sf;
			sf << revert_val;
			max_iter_tctrl->ChangeValue(sf);
		}
	}
	{
		// update var_man with new selection
		int vc_sel = var_choice->GetSelection();
		wxString var_nm = var_choice->GetString(vc_sel);
		TableInterface* table_int = project->GetTableInt();
		int col_id = table_int->FindColId(var_nm);
		wxString var_man_nm0 = var_man.GetName(0);
		if (vc_sel >= 0 && col_id >= 0) {
			if (var_man.GetVarsCount() > 0 &&
					var_man_nm0 != "" && var_man_nm0 != var_nm)
			{
				var_man.RemoveVar(0);
			}
			if (var_man.GetVarsCount() == 0) {
				int time = project->GetTimeState()->GetCurrTime();
				std::vector<double> min_vals;
				std::vector<double> max_vals;
				table_int->GetMinMaxVals(col_id, min_vals, max_vals);
				var_man.AppendVar(var_nm, min_vals, max_vals, time);
			}
		}
	}
	int var_man_cnt = var_man.GetVarsCount();

    
    EndModal(wxID_OK);
    Destroy();
}


void CorrelParamsFrame::OnAllPairsRadioSelected(wxCommandEvent& ev)
{
	if (!all_pairs_rad || !est_pairs_txt || !est_pairs_num_txt ||
			!rand_samp_rad || !max_iter_txt || !max_iter_tctrl) return;
	all_pairs_rad->SetValue(true);
	est_pairs_txt->Enable(true);
	est_pairs_num_txt->Enable(true);
	rand_samp_rad->SetValue(false);
	max_iter_txt->Enable(false);
	max_iter_tctrl->Enable(false);
}

void CorrelParamsFrame::OnRandSampRadioSelected(wxCommandEvent& ev)
{
	if (!all_pairs_rad || !est_pairs_txt || !est_pairs_num_txt ||
			!rand_samp_rad || !max_iter_txt || !max_iter_tctrl) return;
	all_pairs_rad->SetValue(false);
	est_pairs_txt->Enable(false);
	est_pairs_num_txt->Enable(false);
	rand_samp_rad->SetValue(true);
	max_iter_txt->Enable(true);
	max_iter_tctrl->Enable(true);
}

void CorrelParamsFrame::OnBinsTextCtrl(wxCommandEvent& ev)
{
}

void CorrelParamsFrame::OnBinsSpinEvent(wxSpinEvent& ev)
{
}

void CorrelParamsFrame::OnDistanceChoiceSelected(wxCommandEvent& ev)
{
	UpdateThreshTctrlVal();
}

void CorrelParamsFrame::OnThreshCheckBox(wxCommandEvent& ev)
{
	if (!thresh_tctrl || !thresh_slider || !thresh_cbx) return;
	bool checked = thresh_cbx->GetValue();
	thresh_tctrl->Enable(checked);
	thresh_slider->Enable(checked);
	UpdateEstPairs();
}

void CorrelParamsFrame::OnThreshTextCtrl(wxCommandEvent& ev)
{
	if (!thresh_tctrl || !thresh_slider) return;
	wxString val = thresh_tctrl->GetValue();
	val.Trim(false);
	val.Trim(true);
	double t;
	bool is_valid = val.ToDouble(&t);
	if (is_valid) {
		if (t <= GetThreshMin()) {
			thresh_slider->SetValue(0);
		} else if (t >= GetThreshMax()) {
			thresh_slider->SetValue(sldr_tcks);
		} else {
			double s = ((t-GetThreshMin())/(GetThreshMax()-GetThreshMin()) *
									((double) sldr_tcks));
			thresh_slider->SetValue((int) s);
		}
	} else {
		UpdateThreshTctrlVal();
	}
}

void CorrelParamsFrame::OnThreshTctrlKillFocus(wxFocusEvent& ev)
{
	wxString val = thresh_tctrl->GetValue();
	val.Trim(false);
	val.Trim(true);
	double t;
	bool is_valid = val.ToDouble(&t);
	if (is_valid) {
		if (t < GetThreshMin()) {
			wxString s;
			s << GetThreshMin();
			thresh_tctrl->ChangeValue(s);
		} else if (t > GetThreshMax()) {
			wxString s;
			s << GetThreshMax();
			thresh_tctrl->ChangeValue(s);
		}
	} else {
		UpdateThreshTctrlVal();
	}	
}

void CorrelParamsFrame::OnThreshSlider(wxCommandEvent& ev)
{
	if (!thresh_tctrl || !thresh_slider) return;
	UpdateThreshTctrlVal();
	if (thresh_cbx && thresh_cbx->GetValue()) UpdateEstPairs();
}

void CorrelParamsFrame::OnMaxIterTextCtrl(wxCommandEvent& ev)
{
}

void CorrelParamsFrame::OnMaxIterTctrlKillFocus(wxFocusEvent& ev)
{
	wxString val = max_iter_tctrl->GetValue();
	val.Trim(false);
	val.Trim(true);
	long t;
	bool is_valid = val.ToLong(&t);
	if (is_valid) {
		if (t < CorrelParams::min_iter_cnst) {
			wxString s;
			s << CorrelParams::min_iter_cnst;
			max_iter_tctrl->ChangeValue(s);
		} else if (t > CorrelParams::max_iter_cnst) {
			wxString s;
			s << CorrelParams::max_iter_cnst;
			max_iter_tctrl->ChangeValue(s);
		}
	} else {
		wxString s;
		s << CorrelParams::def_iter_cnst;
		max_iter_tctrl->ChangeValue(s);
	}	
}


bool CorrelParamsFrame::IsArc()
{
	if (!dist_choice) {
		return correl_params.dist_metric == WeightsMetaInfo::DM_arc;
	}
	return dist_choice->GetSelection() > 0;
}

bool CorrelParamsFrame::IsMi()
{
	if (!dist_choice) {
		return correl_params.dist_units != WeightsMetaInfo::DU_km;
	}
	return dist_choice->GetSelection() == 1;
}

double CorrelParamsFrame::GetThreshMin()
{
	if (IsArc()) {
		double r = project->GetMin1nnDistArc();
		if (IsMi()) return GenGeomAlgs::EarthRadToMi(r);
		return GenGeomAlgs::EarthRadToKm(r);
	}
	return project->GetMin1nnDistEuc();
}

double CorrelParamsFrame::GetThreshMax()
{
	if (IsArc()) {
		double r = project->GetMaxDistArc();
		if (IsMi()) return GenGeomAlgs::EarthRadToMi(r);
		return GenGeomAlgs::EarthRadToKm(r);
	}
	return project->GetMaxDistEuc();
}

void CorrelParamsFrame::UpdateVarChoiceFromTable(const wxString& default_var)
{
	TableInterface* table_int = project->GetTableInt();
	if (!table_int || !var_choice) return;

	int var_pos = -1;
	var_choice->Clear();
	std::vector<wxString> names;
	table_int->FillNumericNameList(names);
	for (size_t i=0, sz=names.size(); i<sz; ++i) {
		var_choice->Append(names[i]);
		if (names[i] == default_var) var_pos = i;
	}
	if (var_pos >= 0) {
		var_choice->SetSelection(var_pos);
	}
	UpdateApplyState();
}

void CorrelParamsFrame::UpdateApplyState()
{
	if (!var_choice || !apply_btn) return;
	apply_btn->Enable(var_choice->GetSelection() >= 0);
}

void CorrelParamsFrame::UpdateThreshTctrlVal()
{
	if (!thresh_tctrl) return;
	double sf = 0.5;
	if (thresh_slider) {
		sf = (((double) thresh_slider->GetValue()) / ((double) sldr_tcks));
	}
	wxString s;
	double v = GetThreshMin() + (GetThreshMax() - GetThreshMin())*sf;
	s << v;
	thresh_tctrl->ChangeValue(s);
}

void CorrelParamsFrame::UpdateEstPairs()
{
	if (!thresh_cbx || !est_pairs_num_txt) return;
	wxString s;
	long nobs = project->GetNumRecords();
	long max_pairs = (nobs*(nobs-1))/2;
	if (thresh_cbx->GetValue()) {
		double sf = 0.5;
		if (thresh_slider) {
			sf = (((double) thresh_slider->GetValue()) / ((double) sldr_tcks));
		}
		double mn = (double) nobs;
		double mx = (double) max_pairs;
		double est = mn + (mx-mn)*sf;
		long est_l = (long) est;
		s << est_l;
	} else {
		s << max_pairs;
	}
	est_pairs_num_txt->SetLabelText(s);
}

wxString CorrelParamsFrame::GetHelpPageHtml() const
{
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "  <style type=\"text/css\">";
	s << "  body {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    font-size: small;";
	s << "  }";
	s << "  h1 h2 {";
	s << "    font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;";
	s << "    color: blue;";
	s << "  }";
	s <<   "</style>";
	s << "</head>";
	s << "<body>";
	
	s << "<h2>Correlogram Overview</h2>";
	s << "<p>";
	s << "Correlogram visualizes autocorrelation as a function of distance.";
	s << "</p>";

	s << "<h2>Number Bins</h2>";
	s << "<p>";
	s << "The number of distance bands to partition distance range into.";
	s << "</p>";
	
	s << "<h2>Max Distance</h2>";
	s << "<p>";
	s << "Limit the algorithm to look at pairs only within a limited distance ";
	s << "apart.  Note that the maximum distance provided is the exact ";
	s << "maximum distance that any two observation centers are apart.";
	s << "</p>";
	
	s << "<h2>All Pairs</h2>";
	s << "<p>";
	s << "All pairs of observation centers will be sampled exactly once. ";
	s << "The running time of this algorithm is quadratic in the number of ";
	s << "observations, so consider using the Max Distance threshold or ";
	s << "Random Sample for data sets with more than 10,000 observations.";
	s << "The Estimated Pairs gives the number of pairs of centers that will ";
	s << "be involved in the computation.  This is comparable to the ";
	s << "Iterations parameter in the Random Sample method.";
	s << "</p>";

	s << "<h2>Random Sample</h2>";
	s << "<p>";
	s << "Pairs of observations are chosen at random up to the number ";
	s << "of iterations specified.  Random Sampling converges very quickly ";
	s << "to similar values as in All Pairs, but had the advantage of ";
	s << "a constant running time.  This is the only way to handle very ";
	s << "large data sets over the entire distance range.";
	s << "</p>";
	
	s << "</body>";
	s << "</html>";
	return s;
}