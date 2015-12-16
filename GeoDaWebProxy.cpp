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

#include <list>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <boost/thread/thread.hpp>

#include <curl/curl.h>
#include <wx/string.h>
#include <wx/utils.h>

#include "GeoDaWebProxy.h"

#include "Explore/LisaMapNewView.h"
#include "Explore/HistogramView.h"
#include "Explore/MapNewView.h"
#include "Explore/ScatterNewPlotView.h"
#include "Explore/ScatterPlotMatView.h"

#include "Project.h"
#include "FramesManager.h"

using namespace std;

GeoDaWebProxy::GeoDaWebProxy()
{
    
}


GeoDaWebProxy::GeoDaWebProxy(const string& _user_name, const string& _api_key)
{
    user_name = _user_name;
    api_key = _api_key;
    
}

GeoDaWebProxy::~GeoDaWebProxy() {
    
}

void GeoDaWebProxy::Close() {
    
}

void GeoDaWebProxy::Publish(Project* p)
{
	if (p == NULL)
		return;
	
	ostringstream ss;
	
	// table_name
	ss << buildParameter("table_name", p->layername);
	
	// maps & plots
	FramesManager* fm = p->GetFramesManager();
	list<FramesManagerObserver*> observers(fm->getCopyObservers());
	list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (LisaMapFrame* w = dynamic_cast<LisaMapFrame*>(*it)) {
			vector<int> clusters;
			w->GetVizInfo(clusters);
			if (!clusters.empty()) {
				ss << "&" << buildParameter("lisa", clusters);
			}
			continue;
		}
		if (MapFrame* w = dynamic_cast<MapFrame*>(*it)) {
			std::map<wxString, std::vector<int> > colors;
			w->GetVizInfo(colors);
			if (!colors.empty()) {
				wxString map_conf(buildParameter(colors));
				ss << "&" << buildParameter("map", map_conf);
			}
			continue;
		} 
		
		if (HistogramFrame* w = dynamic_cast<HistogramFrame*>(*it)) {
			wxString col_name;
			int num_bins = 0;
			w->GetVizInfo(col_name, num_bins);
			if (!col_name.empty() && num_bins>0) {
				wxString val;
				val << "[\"" << col_name << "\"," << num_bins << "]";
				ss << "&" << buildParameter("histogram", val);
			}
			continue;
		} 
		if (ScatterPlotMatFrame* w = dynamic_cast<ScatterPlotMatFrame*>(*it)) {
			vector<wxString> vars;
			w->GetVizInfo(vars);
			if (!vars.empty()) {
				ss << "&" << buildParameter("scattermatrix", vars);
			}
		    continue;
			
		}
		if (ScatterNewPlotFrame* w = dynamic_cast<ScatterNewPlotFrame*>(*it)) {
			wxString x;
			wxString y;
			w->GetVizInfo(x, y);
			if (!x.empty() && !y.empty()) {
				wxString val;
				val << "[\"" << x << "\",\"" << y << "\"]";
				ss << "&" << buildParameter("scatterplot", val);
			}
			continue;
		} 
	}
	
	// submit request
	string parameter = ss.str();
	
	string returnUrl = doPost(parameter);
	
	// launch browser with return url
	wxString published_url("");
	wxLaunchDefaultBrowser(published_url);
}


void GeoDaWebProxy::SetKey(const string& key) {
    api_key = key;
}

void GeoDaWebProxy::SetUserName(const string& name) {
    user_name = name;
}

string GeoDaWebProxy::buildParameter(map<wxString, vector<int> >& val)
{
	ostringstream par;
	map<wxString, vector<int> >::iterator it;
	
	par << "{" ;
	for (it=val.begin(); it != val.end(); ++it) {
		wxString clr( it->first);
		vector<int>& ids = it->second;
		
		par << "\"" << clr.mb_str() << "\": [";
		for (size_t i=0; i< ids.size(); i++) {
			par << ids[i] << ",";
		}
		par << "],";
	}
	par << "}" ;
	
	return par.str();
}

string GeoDaWebProxy::buildParameter(const char* key, string& val)
{
	ostringstream par;
	par << key << "=" << val;
	return par.str();
}


string GeoDaWebProxy::buildParameter(const char* key, wxString& val)
{
	ostringstream par;
	par << key << "=" << val.mb_str();
	return par.str();
}

string GeoDaWebProxy::buildParameter(const char* key, vector<int>& val)
{
	ostringstream par;
	par << key << "=" << "[";
	for (size_t i=0, n=val.size(); i<n; i++) {
		par << val[i] << ",";
	}
	par << "]";
	return par.str();
}

string GeoDaWebProxy::buildParameter(const char* key, vector<wxString>& val)
{
	ostringstream par;
	par << key << "=" << "[";
	for (size_t i=0, n=val.size(); i<n; i++) {
		par << "\"" << val[i] << "\",";
	}
	par << "]";
	return par.str();
}
									 
string GeoDaWebProxy::buildBaseUrl()
{
    ostringstream url;
    url << "https://webpool.csf.asu.edu/xun/myapp/index/";
    return url.str();
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    string data((const char*) ptr, (size_t) size * nmemb);
    *((stringstream*) stream) << data << endl;
    return size * nmemb;
}


void GeoDaWebProxy::doGet(string parameter)
{
    CURL* curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if (curl) {
        string url = buildBaseUrl() + "?api_key=" + api_key +"&" + parameter;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        
        // Grab image 
        res = curl_easy_perform(curl);
        if( res ) {
            printf("Cannot connect cartodb.com!\n");
        } 
        
        int res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %d\n", res_code);
			// Clean up the resources 
			curl_easy_cleanup(curl);
            return;
        }
		// Clean up the resources 
		curl_easy_cleanup(curl);
    }
    
}

string GeoDaWebProxy::doPost(string parameter)
{
    CURL* curl;
    CURLcode res;

    //curl_global_init(CURL_GLOBAL_ALL);
    stringstream out;
	
    curl = curl_easy_init();
    if (curl) {
        string url = buildBaseUrl();
        parameter = "api_key=" + api_key + "&" + parameter;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, parameter.c_str());
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		
		stringstream out;
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
        
        // Grab image 
        res = curl_easy_perform(curl);
        if( res ) {
            printf("Cannot connect cartodb.com!\n");
        } 
        
        int res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %d\n", res_code);
			// Clean up the resources 
			curl_easy_cleanup(curl);
            return "";
        }
		// Clean up the resources 
		curl_easy_cleanup(curl);
    }
    
   
    //curl_global_cleanup();
    return out.str();
}