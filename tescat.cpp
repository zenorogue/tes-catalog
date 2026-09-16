#include <emscripten.h>
#include <vector>
#include <map>
#include <string>
#include <time.h>
#include <cctype>
#include <emscripten/fetch.h>

#define CAP_INV 0
#define CAP_URL 1
#define ISWEB 1
#define GLES_ONLY
// #define MAXMDIM 3
#define CAP_GD 0
#define CAP_COMPLEX2 0
#define CAP_LEGACY 0
#define NOMAIN
#define HYPERWEB_ONLY_FUNCTIONS

#include "hr/hyper.cpp"
#include "hr/hyperweb.cpp"

using namespace std;

struct tesdata {
  int type;
  const char *fname;
  const char *label;
  const char *desc;
  int kind;
  const char *link;
  };

tesdata alldata[] = {
#include "table.cpp"
#include "table-arcm.cpp"
#include "table-upto5.cpp"
/*
{-1, "pseudo-Archimedean/by-tile/5444/4445_4", "(4,4,4,5) x4", "", 0, "bf/8d/RydoKPns"},
{-1, "hr/polyforms-3737/5-16 + 5-38/3737 5-16 + 5-38 s33", "3737 polyforms, 5-16 + 5-38, s33", "", 0, "8d/d8/Zhxu7fqT"},
{-1, "hr/polyforms-3737/5-16 + 5-38/3737 5-16 + 5-38 s07", "3737 polyforms, 5-16 + 5-38, s7", "", 0, "2c/40/gTTzfVte"},
{-1, "hr/polyforms-3737/5-16 + 5-38/3737 5-16 + 5-38 s02", "3737 polyforms, 5-16 + 5-38, s2", "", 0, "73/ea/Lrlf25RP"},
{-1, "hr/polyforms-3737/5-16 + 5-38/3737 5-16 + 5-38 s31", "3737 polyforms, 5-16 + 5-38, s31", "", 0, "ad/d8/B55eTEAP"},
{-1, "multitile/3-11/2+2/3-11-2f+2f-06", "{3,11}, diamond 1F + diamond 1F, solution 6", "", 0, "be/96/F6yQHvkv"},
{-1, "multitile/3-11/2+2/3-11-2f+2f-04", "{3,11}, diamond 1F + diamond 1F, solution 4", "", 0, "25/21/smgvLYGW"},
{-1, "twobrid/4488twobrid/4488twobrid 5-5-1", "(4,4,8,8) twobrid, (4s,8s,4s,8s)+(4s,8s,[8l])+(4s,8s,8l,4l,8l)+([4l],[8l])x2", "", 0, "48/48/uCGF4kwY"},
{-1, "other/45halfdomino/halfdomino45grid-15", "{4,5} half-domino, grid 15", "", 0, "b6/31/KDjSKNKV"},
{-1, "pseudo-Archimedean/356i hybrid/3669/10/2 (55)/356i 4a5 5a2 5b3 162", "(3,5,6,18) hybrid, (3,6,6,9)x5, (3,6,9,6)Ax2, (3,6,9,6)Fx3", "", 0, "24/28/cf0TtdvP"},
{-1, "archimedean/6-valent/3-3-4-4-6-6/mFCFDCD", "(6,3,6,4,3,4)", "edge=1.71911 tiles=6 dual=4", 1, "90/d6/cbYDihpA"},
{0, "sample/brickwork", "brickwork", "a simple test<br/>", 0, "8d/1c/ropBeVXQ"},
*/
};

void set_value(string name, string s);

bool hr_initialized;

string pattern = "mirror";
string projh = "poincare";
string proje = "medium";
string projs = "ortho";

void ensure_hr_initialized() {
  if(hr_initialized) return;
  hr_initialized = true;

  // for(auto& w: *hr::all_debugflags) w.second->enabled = true;
  // hr::debug_memory_cell.enabled = false;

  printf("initializing the HR configuration\n");
  hr::init_floorcolors();
  hr::initConfig();
  printf("initializing the HR settings\n");
  hr::geometry = hr::gNormal;
  hr::variation = hr::eVariation::pure;
  hr::check_cgi();
  hr::cgip->require_basics();
  hr::cgip->require_shapes();
  hr::rulegen::auto_rulegen = false;
  hr::svg::svg_mode = 1;

  hr::vid.use_smart_range = 2;
  hr::vid.smart_area_based = true;
  hr::vid.smart_range_detail = 1;
  hr::vid.cells_generated_limit = 2000;
  hr::modelcolor = 0x000000FF;

  hr::backcolor = 0xFFFFFFFF;
  // hr::bordcolor = 0xFFFFFFFF;
  // hr::forecolor = 0;

  hr::shot::shotx = 200;
  hr::shot::shoty = 200;

  hr::pconf.scale = 0.95;

  hr::firstland = hr::specialland = hr::laCanvas;
  hr::randomPatternsMode = false;
  hr::land_structure = hr::lsSingle;
  
  hr::ccolor::which = &hr::ccolor::shape_mirror;

  hr::mapeditor::drawplayer = false;
  hr::global_boundary_ratio = 0.25;

  hr::peace::on = true;

  hr::alt_distlimit = 0;
  hr::alt_BARLEV = 8;
  }

bool currently_rendering = false;

vector<tesdata*> tesdata_to_read;
vector<tesdata*> render_delayed;

const int ANY = -2;
int curreq = ANY;

void set_value(string name, string s) {
  EM_ASM_({
    var name = UTF8ToString($0, $1);
    var value = UTF8ToString($2, $3);
    document.getElementById(name).innerHTML = value;
    }, name.c_str(), int(name.size()),
    s.c_str(), int(s.size())
    );
  }

bool does_element_exist(string name) {
  return EM_ASM_INT({
    var name = UTF8ToString($0, $1);
    return document.getElementById(name) ? 1 : 0;
    }, name.c_str(), int(name.size()));
  }

void delay_render(string lnk) {
  EM_ASM_({
    var name = UTF8ToString($0, $1);
    setTimeout(function() { render(name); }, 500);
    }, lnk.c_str(), int(lnk.size()));
  }

map<string, string> link_to_tesfile;

tesdata *tes_by_link(const string& s) {
  int numdata = sizeof(alldata) / sizeof(tesdata);
  for(int i=0; i<numdata; i++) if(alldata[i].link == s) return &alldata[i];
  return nullptr;
  }

void enter_tessellation(tesdata *td) {
  ensure_hr_initialized();

  printf("stopping game\n");
  hr::stop_game();

  printf("creating normal geometry\n");
  hr::geometry = hr::gNormal;
  hr::check_cgi();
  hr::cgip->require_basics();

  using namespace hr;
  if(pattern == "mirror")
    hr::ccolor::which = &hr::ccolor::shape_mirror;
  if(pattern == "shape")
    hr::ccolor::which = &hr::ccolor::shape;
  if(pattern == "sides")
    hr::ccolor::which = &hr::ccolor::sides;
  if(pattern == "white")
    hr::ccolor::which = &hr::ccolor::plain, hr::ccolor::rwalls = 0, ccolor::plain.ctab = {0xFFFFFF}; 
  if(pattern == "random")
    hr::ccolor::which = &hr::ccolor::random, hr::ccolor::rwalls = 0;

  if(td->kind == 0) {
    printf("creating the input.tes file for %s\n", td->fname);
    FILE *f = fopen("input.tes", "wt");
    fprintf(f, "%s", link_to_tesfile[td->link].c_str());
    fclose(f);

    printf("running the tessellation\n");
    hr::arb::run("input.tes");
    unlink("input.tes");
    }
  else {
    hr::variation = hr::eVariation::pure;
    hr::arcm::load_symbol(td->label, true);
    hr::start_game();
    }

  string choice = euclid ? proje : sphere ? projs : projh;

  if(choice == "poincare" || choice == "stereo") pmodel = mdDisk, pconf.alpha = 1, pconf.scale = 0.95;
  if(choice == "klein" || choice == "gnomonic") pmodel = mdDisk, pconf.alpha = 0, pconf.scale = 0.95;
  if(choice == "gans") pmodel = mdDisk, pconf.alpha = 999, pconf.scale = 200;
  if(choice == "aed") pmodel = mdEquidistant, pconf.scale = 0.75;
  if(choice == "aea") pmodel = mdEquiarea, pconf.scale = 0.75;
  if(choice == "egg") pmodel = mdConformalEgg, pconf.scale = 0.9;
  if(choice == "ortho") pmodel = mdDisk, pconf.alpha = 999, pconf.scale = 950;
  if(choice == "large") pmodel = mdDisk, pconf.alpha = 1, pconf.scale = 0.95;
  if(choice == "medium") pmodel = mdDisk, pconf.alpha = 1, pconf.scale = 0.5;
  if(choice == "small") pmodel = mdDisk, pconf.alpha = 1, pconf.scale = 0.25;
  if(choice == "mercator") pmodel = mdBand, pconf.alpha = mdBand, pconf.scale = 0.5;
  if(choice == "band") pmodel = mdBand, pconf.alpha = mdBand, pconf.scale = 1;
  if(choice == "halfplane") pmodel = mdHalfplane, pconf.alpha = 1, pconf.scale = 0.95;
  if(choice == "square") pmodel = mdConformalSquare, pconf.alpha = 1, pconf.scale = 0.95;

  pattern = "";
  }

string render_tessellation() {

  hr::dynamicval<int> db(hr::floorshapes_level, 1);

  printf("taking the screenshot\n");
  hr::shot::format = hr::shot::screenshot_format::svg;
  hr::shot::take("input.svg");

  printf("cellcount = %d\n", hr::cellcount);
  string ret = hr::svg::sout.s;
  hr::svg::sout.s = "";
  return ret;
  }

bool needs_tesfile(tesdata *td) {
  if(td->kind == 0 && !link_to_tesfile.count(td->link)) return true;
  return false;
  }

string imglink(tesdata* t) {
  string link = t->link;
  if(needs_tesfile(t)) {
    tesdata_to_read.push_back(t);
    return "<div id='" + link + "'>(loading)</div>";
    }
  else {
    render_delayed.push_back(t);
    return "<div id='" + link + "'>(rendering)</div>";
    }
  }

void delayed_render(const string& lnk) {
  if(does_element_exist(lnk)) {
    if(currently_rendering) { set_value(lnk, "waiting"); delay_render(lnk); return; }
    hr::dynamicval<bool> cr(currently_rendering, true);
    enter_tessellation(tes_by_link(lnk));
    set_value(lnk, render_tessellation());
    }
  }

void tesdata_succeeded(emscripten_fetch_t *fetch) {
  const char *lnk = (const char*) fetch->userData;
  string content(fetch->data, fetch->numBytes);
  emscripten_fetch_close(fetch);

  link_to_tesfile[lnk] = content;

  if(does_element_exist(lnk)) {
    if(currently_rendering) { set_value(lnk, "waiting"); delay_render(lnk); return; }
    hr::dynamicval<bool> cr(currently_rendering, true);
    enter_tessellation(tes_by_link(lnk));
    set_value(lnk, render_tessellation());
    }
  }

void tesdata_failed(emscripten_fetch_t *fetch) {
  const char *lnk = (const char*) fetch->userData;
  set_value(lnk, "[failed]");
  emscripten_fetch_close(fetch);
  }

void tesdata_progress(emscripten_fetch_t *fetch) { }

void read_tesdata(tesdata& t) {
  set_value(t.link, "[loading]");

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
  attr.userData = (void*) t.link;
  attr.onsuccess = tesdata_succeeded;
  attr.onerror = tesdata_failed;
  attr.onprogress = tesdata_progress;

  string full_fname = "https://zenorogue.github.io/tes-catalog/files/tessellations/";
  full_fname += t.fname;
  full_fname += ".tes";

  emscripten_fetch(&attr, full_fname.c_str());
  }

void set_location(string s) {
  EM_ASM_({
    var value = UTF8ToString($0, $1);
    window.history.pushState(value, document.title + ":"+value, "?c=" + encodeURIComponent(value));
    }, s.c_str(), int(s.size())
    );
  }

string genlink(string to, string cap) {
  // return "<a href=\"./?c=" + to + "\">" + cap + "</a>";
  return "<a href=\"javascript:jump('" + to + "')\">" + cap + "</a>";
  }

vector<tuple<string, string, int> > restrictions = {
  {"", "any", ANY}, {"H/", "hyperbolic", -1}, {"E/", "Euclidean", 0}, {"S/", "spherical", 1}
  };

int numstrcmp(const char *a, const char *b) {
  if(isdigit(*a) && isdigit(*b)) {
    int va = 0, vb = 0;
    while(isdigit(*a)) va = 10*va + *(a++) - '0';
    while(isdigit(*b)) vb = 10*vb + *(b++) - '0';
    if(va != vb) return va - vb;
    return numstrcmp(a, b);
    }
  else if(*a != *b) return *a - *b;
  else if(*a == 0) return 0;
  else return numstrcmp(a+1, b+1);
  }

string seekstr;

bool at(const char *fname, const string& needle) {
  for(char c: needle) if(*(fname++) != c) return false;
  return true;
  }

void generate_page(string s) {
  set_location(s);
  srand(time(NULL));  
  string out;
  string parent;
  
  int numdata = sizeof(alldata) / sizeof(tesdata);

  sort(alldata, alldata + numdata, [] (const tesdata& d1, const tesdata& d2) { return numstrcmp(d1.fname, d2.fname) < 0; });
  
  string prefix;
  curreq = ANY;
  
  
  // string advsearch, advsearch_str;
  
  for(auto&[code, full, val]: restrictions) {
    if(code != "" && s.substr(0, 2) == code) {
      curreq = val;
      s = s.substr(2);
      prefix = code;
      }
    }
  
  /*
  if(s.substr(0, 7) == "search/") {
    int i = 7;
    while(i < int(s.size()-1) && (s[i] != '/' || s[i+1] != '/')) i++;
    advsearch = s.substr(7, i-7);
    advsearch_str = "search/" + advsearch + "//";
    if(i <= int(s.size() - 2))
      s = s.substr(i+2);
    }
  */

  while(s != "" && s[0] == '/') s = s.substr(1);
  
  if(s != "") {
    for(int i=0; i<int(s.size())-1; i++) if(s[i] == '/') parent = s.substr(0, i+1);
    out += genlink(prefix+/*advsearch_str+*/parent, "go back!") + "<br/><br/>";
    }
  
  int len = s.size();
  
  vector<tesdata*> matching;
  matching.reserve(numdata);

  tesdata_to_read.clear();
  render_delayed.clear();

  for(int i=0; i<numdata; i++) {
    auto& td = alldata[i];
    if(td.type != curreq && curreq != ANY) continue;
    if(at(td.fname, s)) 
      matching.push_back(&td);
    }

  if(s == "") {
    out += "An online version of the catalog of tessellations, compiled by Marek Čtrnáct.<br/><br/>";
    out += "<ul>\n"
      "<li> Links run the tessellation in the <a href=\"http://roguetemple.com/z/hyper/\">HyperRogue engine</a>, where you can view it from other angles, or make a "
      "high quality SVG screenshot (press shift+A)."
      "<li> All the tessellation files can be downloaded <a href=\"https://github.com/zenorogue/tes-catalog/releases\">here</a>."
      "<li> In Archimedean tilings, the notation like \"A:(4,4); B:(4,4); \"(B,B,A,A,A) (1)(4)\" means that A and B can get any value which is at least 4 and divisible by 4.";

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    if(tm.tm_mon == 5 && tm.tm_mday == 17)
      out += "<li> <b>Happy World Tessellation Day!</b><br/>";
    
    out +=
      "<li> Join the <a href=\"https://discord.gg/SnuhaW8\">#tessellation channel on the HyperRogue discord</a> for discussions!<br/>"
      "<li> See <a href=\"https://github.com/zenorogue/tes-catalog\">here</a> for raw tessellation files, the source code of this website, and citation info!<br/>"
      "</ul><br/><br/>";
    }

  out += "Curvature:";
  bool next = false;
  string current;
  for(auto& [code, full, val]: restrictions) {
    if(next) out += " | ";
    next = true;
    out += genlink(code + seekstr + s, full);
    if(val == curreq) current = full;
    }
  out += " (currently showing: " + current + ")<br/><br/>";

  // out += "Displaying advanced search results for: '" + advsearch + "'<br/><br/>";

  out += "Advanced search: <input id=\"search\" type=\"text\"><button onClick=\"do_advanced_search('" + prefix + "', '" + s + "')\">Search!</button><br/><br/>";
    
  out += "Number of tessellations here: " + std::to_string(matching.size()) + "<br/><br/>";    

  out += "<a href=\"javascript:options('" + s + "');\">change graphical settings</a><br/><br/>";
          
  string last_explored = "?";

  
  for(int ti=0; ti<(int) matching.size(); ti++) {
    auto& td = *matching[ti];
    const string& fname = td.fname;
    const string& label = td.label;
    const string& desc = td.desc;

    for(int i=len; i<int(fname.size()); i++) if(fname[i] == '/') {
      string where = fname.substr(0, i+1);
      if(true) {
        int wlen = where.size();
        vector<tesdata*> justhere;
        int cnt = 0;
        
        map<string, vector<tesdata*>> per_group;

        while(ti < (int) matching.size()) {
          auto& td1 = *matching[ti];
          if(!at(td1.fname, where)) break;
          cnt++;
          const string& fname1 = td1.fname;
          for(int i=wlen; i<int(fname1.size()); i++) if(fname1[i] == '/') {
            per_group[fname1.substr(wlen, i-wlen)].push_back(&td1);
            goto next_td1;
            }
          justhere.push_back(&td1);
          next_td1: ti++;
          }
        ti--;
        
        vector<vector<tesdata*>> groups;
        for(auto& pp: per_group) groups.push_back(std::move(pp.second));
        sort(groups.begin(), groups.end(), [] (auto a, auto b) { return a.size() > b.size(); });

        auto pick_one = [&] (vector<tesdata*>& g) {
          swap(g[rand() % g.size()], g.back());
          auto ret = g.back();
          g.pop_back();
          return ret;
          };

        for(auto& p: groups) justhere.push_back(pick_one(p));

        for(int it=0; it<10; it++) for(auto& p: groups) if(p.size() && justhere.size() < 10)
          justhere.push_back(pick_one(p));

        string images = "";
        int left = 10;
        int n = justhere.size();
        images += "<table><tr>";
        for(auto j: justhere) {
          if(rand() % n < left) {
            string cu = j->fname;
            int lastslash = 0; for(int u=0; u<int(cu.size()); u++) if(cu[u] == '/') lastslash = u+1;
            images += "<td>" + genlink(cu.substr(0, lastslash), imglink(j)) + "</td>";
            left--;
            if(left == 5) images += "</tr><tr>";
            }
          n--;
          }
        images += "</tr></table>";

        out += "<h2>" + genlink(prefix+/*advsearch_str+*/where, where) + " (" + std::to_string(cnt) + " tessellations)</h2>";
        out += images;
        out += "<br/><br/>";
        }
      goto next_td;
      }
    
    if(1) {
      string cline = 
        "view.html?c=-viz";
      
      cline += "+-back+ffffff+-fore+0+-borders+ffffff+-fillmodel+ff";
      
      if(td.type >= 0) 
        cline += "+-zoom+.95";

      // cline += "+-wsh+9+-palrgba+sub+00000020+-palrgba+normal+000000FF+-smart+1";
      cline += "+-smart+1";
      
      if(td.kind == 1) 
        cline += "+-canvas+B";
      else
        cline += "+-canvas+A";
        
      if(td.kind == 0) {      
        cline += "+-arbi+1&1=tessellations%2F";
        for(char ch:fname)
          if(ch == '+') cline += "%2B";
          else if(ch == '/') cline += "%2F";
          else cline += ch;
        cline += ".tes";
        }
      else {
        cline += "+-7+-symbol+\"";
        string lab = td.label;
        for(char c: lab)
          if(c == '(') cline += "%28";
          else if(c == ')') cline += "%29";
          else if(c == '[') cline += "%5B";
          else if(c == ']') cline += "%5D";
          else if(c == ',') cline += "%2C";
          else if(c == ' ') cline += "%2C";
          else cline += c;
        cline += "\"";
        }
  
      char buf[9999];
      snprintf(buf, 9999, "<table><tr><td>%s</td><td><b>%s</b> <a href='%s'>(play online)</a>",
        imglink(&td).c_str(),
        label.c_str(),
        cline.c_str());
      out += buf;
      
      if(td.kind == 0) {
        snprintf(buf, 9999, "<a href=\"%s\">(download)</a>",("files/tessellations/" + fname + ".tes").c_str());
        out += buf;
        }
      else out += " (Archimedean)";
      
      snprintf(buf, 9999, "<br/><br/>%s</td></tr></table>\n",
        desc.c_str()
        );
      out += buf;
      }
    
    next_td: ;
    }

  set_value("all", out);

  for(auto td: tesdata_to_read) read_tesdata(*td);
  for(auto td: render_delayed) delay_render(td->link);
  }

void play_tessellation(const string &s, const string &lnk) {
  tesdata *which = tes_by_link(lnk);

  stringstream play_page;
  if(!which) {
    play_page <<
      "Illegal tessellation!</br!>"
      "<input type='button' value='go back' onclick=\"jump('" + s + "')\"/>";
    set_value("all", play_page.str());
    }
  else {

    EM_ASM_({
      canvas = document.getElementById('canvas');
      canvas.style.display = "";
      Module['canvas'] = canvas;
      });

    play_page <<
      "<span id='controls'>"
        "<span><input type='checkbox' id='resize'>Resize canvas</span>"
        "<span><input type='checkbox' id='pointerLock' checked>Lock/hide mouse pointer &nbsp;&nbsp;&nbsp;</span>"
        "<span><input type='button' value='Fullscreen' onclick=\"Module.requestFullscreen(document.getElementById('pointerLock').checked, document.getElementById('resize').checked)\">"
        "<input type='button' value='go back' onclick=\"close_gfx(); jump('" + s + "')\"/>"
        "</span>";
    set_value("all", play_page.str());
    enter_tessellation(tes_by_link(lnk));
    hr::svg::svg_mode = 2;
    if(!hr::graphics_on) hr::init_graph();
    hr::mainloop();
    hr::popScreenAll();
    hr::clearMessages();
    }  
  }

void view_option_screen(const char *s) {
  stringstream ss;

  ss << "<input id=\"width\" size=10 value='"<< hr::global_boundary_ratio<<"' type=text/> width: bigger = wider cell boundaries<br/>";
  ss << "<input id=\"shotx\" size=10 value='"<< hr::shot::shotx<<"' type=text/> image X size<br/>";
  ss << "<input id=\"shoty\" size=10 value='"<< hr::shot::shoty<<"' type=text/> image Y size<br/>";
  ss << "<input id=\"detail\" size=10 value='"<< hr::vid.smart_range_detail <<"' type=text/> detail: smaller = more detail<br/>";

  auto list = [&] (string title, std::initializer_list<const char*> l) {
    for(auto w: l) ss << " <a href=\"javascript: document.getElementById('" << title << "').value='" << w << "'; void(0);\">" << w << "</a>";
    return "";
    };

  ss << "<input id=\"pattern\" size=10 value='"<< pattern <<"' type=text/> pattern: " << list("pattern", {"shape", "mirror", "sides", "white", "random"}) << "<br/>";
  ss << "<input id=\"ph\" size=10 value='"<< projh <<"' type=text/> hyperbolic projection: " << list("ph", {"poincare", "klein", "gans", "halfplane", "square", "aed", "aea", "egg", "band"}) << "<br/>";
  ss << "<input id=\"pe\" size=10 value='"<< proje <<"' type=text/> Euclidean projection: " << list("pe", {"large", "medium", "small"}) << "<br/>";
  ss << "<input id=\"ps\" size=10 value='"<< projs <<"' type=text/> Spherical projection: " << list("ps", {"ortho", "stereo", "gnomonic", "mercator"}) << "<br/>";

  ss << "<br/><br/>";
  ss << "<br/><br/>";

  ss << genlink(s, "go back without changes") + "<br/><br/>";

  ss << "<a href=\"javascript:activate(); jump('" << s << "')\">activate these changes</a>";

  set_value("all", ss.str());
  }

extern "C" {
  void doit(const char *s) {
    generate_page(s);
    }

  void render(const char *s) {
    delayed_render(s);
    }

  void options(const char *s) {
    view_option_screen(s);
    }

  void activ(double w, int sx, int sy, double det, const char *pat, const char *ph, const char *pe, const char *ps) {
    hr::global_boundary_ratio = w;
    hr::shot::shotx = sx;
    hr::shot::shoty = sy;
    hr::vid.smart_range_detail = det;
    pattern = pat; projh = ph; proje = pe; projs = ps;
    }

  }

