#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <utility>

#include "problem_c.h"
//#include "capacitance.h"

using namespace std;

//#define CHECKER_MODE 1
//#define OUTPUT_ALL 1

void ReadConfig()
{
    ifstream file(config_file);

    string str;
    while (file >> str) {
        if (str == "design:") {
            file >> circuit_file;
        } else if (str == "output:") {
            file >> output_file;
        } else if (str == "rule_file:") {
            file >> rule_file;
        } else if (str == "process_file:") {
            file >> process_file;
        } else if (str == "critical_nets:") {
            getline(file, str);
            char *ch = strtok((char *)str.c_str(), " ");
            while (ch != NULL) {
                critical_nets.insert(atoi(ch));
                ch = strtok(NULL, " ");
            }
        } else if (str == "power_nets:") {
            file >> power_net;
        } else if (str == "ground_net:") {
            file >> ground_net;
        }
    }

    file.close();
}

void ReadCircuit()
{
    ifstream file(path + circuit_file);

    string str;
    getline(file, str);
    size_t pos = str.find(" ");
    cb.bl_x = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(" ");
    cb.bl_y = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(" ");
    cb.tr_x = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    pos = str.find(";");
    cb.tr_y = atoi((char *)str.substr(0, pos).c_str());
    str.erase(0, pos + 1);
    cb.width_x = cb.tr_x - cb.bl_x;
    cb.width_y = cb.tr_y - cb.bl_y;

    Layout l;
    layouts.emplace_back(l); // dummy layout to align id and index;
    while (file >> l.id >> l.bl_x >> l.bl_y >> l.tr_x >> l.tr_y >> l.net_id >> l.layer >> str) {
        l.bl_x -= cb.bl_x;
        l.bl_y -= cb.bl_y;
        l.tr_x -= cb.bl_x;
        l.tr_y -= cb.bl_y;
        if (str == "Drv_Pin")
            l.type = 0;
        else if (str == "Normal" || str == "normal")
            l.type = 1;
        else if (str == "Load_Pin")
            l.type = 2;
        else if (str == "Fill")
            l.type = 3;
        l.is_critical = critical_nets.find(l.net_id) != critical_nets.end() ? 1 : 0;

        layouts.emplace_back(l);
    }

    file.close();
    
    total_metals = layouts.size() - 1;
    total_layers = layouts[total_metals - 1].layer;
}

void ReadProcess()
{
    ifstream file(path + process_file);

    string str;
    getline(file, str); // comment line
    getline(file, str);
    window_size = atoi((char *)str.substr(str.find(" ") + 1, str.length()).c_str());
    getline(file, str); // comment line
    getline(file, str); // comment line

    getline(file, str); // col indices line
    for (int row = 0; row < total_layers + 1; row++) {
        getline(file, str);
        for (int col = 0; col < total_layers; col++) {
            size_t start = str.find("(");
            size_t mid = str.find(",");
            size_t end = str.find(")");
            area_tables.emplace_back(str.substr(start + 1, mid - start - 1));
            fringe_tables.emplace_back(str.substr(mid + 2, end - mid - 2));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }
    }

    // useless lines
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);

    int total_area_tables = 0;
    for (int i = 1; i <= total_layers; i++)
        total_area_tables += i;
    int total_lateral_tables = total_layers;
    int total_fringe_tables = fringe_tables.size() - total_layers * 2;

    string table_name;
    for (int i = 0; i < total_area_tables; i++) {
        AreaTable atbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            atbl.s.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        while (start != string::npos) {
            atbl.a.emplace_back(atof((char *)str.substr(start + 1, mid - start - 1).c_str()));
            atbl.b.emplace_back(atof((char *)str.substr(mid + 2, end - mid - 2).c_str()));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
            start = str.find("(");
            mid = str.find(",");
            end = str.find(")");
        }

        getline(file, str); // empty line

        area_table_map[table_name] = atbl;
    }

    // empty lines
    getline(file, str);
    getline(file, str);
    getline(file, str);

    for (int i = 0; i < total_lateral_tables; i++) {
        FringeTable ftbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            ftbl.d.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        if (start != string::npos) {
            ftbl.a.emplace_back(stod(str.substr(start + 1, mid - start - 1)));
            ftbl.b.emplace_back(stod(str.substr(mid + 2, end - mid - 2)));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }

        getline(file, str); //empty line

        fringe_table_map[table_name] = ftbl;
    }

    // empty lines
    getline(file, str);
    getline(file, str);
    getline(file, str);
    getline(file, str);

    for (int i = 0; i < total_fringe_tables; i++) {
        FringeTable ftbl;
        file >> str >> table_name;
        getline(file, str); // previous line
        getline(file, str); // comment line

        getline(file, str);
        char *ch = strtok((char *)str.c_str(), " ");
        while (ch != NULL) {
            ftbl.d.emplace_back(atof(ch));
            ch = strtok(NULL, " ");
        }

        getline(file, str);
        size_t start = str.find("(");
        size_t mid = str.find(",");
        size_t end = str.find(")");
        if (start != string::npos) {
            ftbl.a.emplace_back(stod(str.substr(start + 1, mid - start - 1)));
            ftbl.b.emplace_back(stod(str.substr(mid + 2, end - mid - 2)));
            str[start] = ';';
            str[mid] = ';';
            str[end] = ';';
        }

        getline(file, str); //empty line

        fringe_table_map[table_name] = ftbl;
    }

    file.close();
}

void ReadRule()
{
    ifstream file(path + rule_file);

    Rule r;
    string str;
    long long w_area = (long long)window_size * window_size;
    long long qw_area = (long long)stride * stride;
    while (file >> r.layer >> str >> r.min_width >> r.min_space >> r.max_fill_width >> 
           r.min_density >> r.max_density)
        rules.emplace_back(r);

    file.close();
}

void AnalyzeDensity()
{
    stride = window_size / 2;
    long long w_area = (long long)window_size * window_size; 
    long long qw_area = (long long)stride * stride;
    // # windows
    window_x = cb.width_x / window_size * 2 - 1;
    window_y = cb.width_y / window_size * 2 - 1;
    // # quarter-windows
    qwindow_x = cb.width_x / stride;
    qwindow_y = cb.width_y / stride;

    int current_metal = 1;
    for (int layer = 1; layer <= total_layers; layer++) {
        // initialize quarter-window
        vector<QuarterWindow> qwindows(qwindow_x * qwindow_y);
        for (int x = 0; x < qwindow_x; x++) {
            int start = x * stride;
            int end = (x + 1) * stride;
            for (int y = 0; y < qwindow_y; y++) {
                int index = x * qwindow_y + y;
                qwindows[index].index = index;
                qwindows[index].area = 0;
                qwindows[index].bl_x = start;
                qwindows[index].tr_x = end;
                qwindows[index].bl_y = y * stride;
                qwindows[index].tr_y = (y + 1) * stride;
            }
        }

        // layout corrsponds to metal index
        Layout temp = layouts[current_metal];
        // index of metal in which quarter-window
        int x_from, x_to, y_from, y_to;
        // coordinate of partial metal in quarter window
        int x_start, x_end, y_start, y_end;
        long long area = 0;
        while (layer == temp.layer) {
		    x_from = temp.bl_x / stride;
		    x_to = (temp.tr_x - 1) / stride;
		    y_from = temp.bl_y / stride;
		    y_to = (temp.tr_y - 1) / stride;
		    for (int x = x_from; x <= x_to; x++) {
			    x_start = x == x_from ? temp.bl_x : x * stride;
			    x_end = x == x_to ? temp.tr_x : (x + 1) * stride;
			    for (int y = y_from; y <= y_to; y++) {
				    y_start = y == y_from ? temp.bl_y : y * stride;
				    y_end = y == y_to ? temp.tr_y : (y + 1) * stride;
				    area = (x_end - x_start) * (y_end - y_start);
				    qwindows[x * qwindow_y + y].area += area;
                    qwindows[x * qwindow_y + y].contribute_metals.emplace_back(temp.id);
			    }
		    }
            if (++current_metal == total_metals + 1)
                break;
            temp = layouts[current_metal];
        }

        vector<Window> ws(window_x * window_y);
        long long min_area = w_area * rules[layer - 1].min_density;
        double min_density = rules[layer - 1].min_density;
        for (int x = 0; x < window_x; x++) {
            int x_start = x * stride;
            int x_end = x_start + window_size;
            for (int y = 0; y < window_y; y++) {
                // window index
                int index = x * window_y + y;
                // quarter window index
                int q_index = x * qwindow_y + y;

                ws[index].index = index;
                ws[index].area = qwindows[q_index].area + qwindows[q_index + qwindow_y].area +
                                 qwindows[q_index + 1].area + qwindows[q_index + qwindow_y + 1].area;
                ws[index].area_insufficient = 0;
                ws[index].density = (double)ws[index].area / w_area;
                ws[index].bl_x = x_start;
                ws[index].tr_x = x_end;
                ws[index].bl_y = y * stride;
                ws[index].tr_y = ws[index].bl_y + window_size;

                // remember which quarter window is included
                ws[index].included_qwindow.emplace_back(q_index);
                ws[index].included_qwindow.emplace_back(q_index + 1);
                ws[index].included_qwindow.emplace_back(q_index + qwindow_y);
                ws[index].included_qwindow.emplace_back(q_index + qwindow_y + 1);

                // remember which window will be affected
                qwindows[q_index].affected_window.emplace_back(index);
                qwindows[q_index + qwindow_y].affected_window.emplace_back(index);
                qwindows[q_index + 1].affected_window.emplace_back(index);
                qwindows[q_index + qwindow_y + 1].affected_window.emplace_back(index);

                if (ws[index].density < min_density) {
                    ws[index].area_insufficient = min_area - ws[index].area;
                #ifdef CHECKER_MODE
                    printf("window %d(%d) (%d %d %d %d): area=%lld, insuff=%lld\n", index, layer, cb.bl_x + x * stride, cb.bl_y + y * stride,
                           cb.bl_x + x * stride + window_size, cb.bl_y + y * stride + window_size,
                           ws[index].area, ws[index].area_insufficient);
                #endif
                }
            }
        }

        quarter_windows.emplace_back(qwindows);
        windows.emplace_back(ws);
    }
}

struct rect_sort_order {
    bool operator()(Rect r1, Rect r2) const {
        if (r1.near_criticals != r2.near_criticals)
            return r1.near_criticals < r2.near_criticals;
        else
            return (long long)r1.width_x * r1.width_y > (long long)r2.width_x * r2.width_y;
    }
} RectSortOrder;

void FindSpace(vector<Rect> &rts, const set<int> &contribute_metals, const int min_width, const int half_min_space)
{
    int actual_min_width = min_width + 2 * half_min_space;

    for (set<int>::iterator it = contribute_metals.begin(); it != contribute_metals.end(); it++) {
        Layout temp = layouts[*it];
        temp.bl_x -= half_min_space;
        temp.bl_y -= half_min_space;
        temp.tr_x += half_min_space;
        temp.tr_y += half_min_space;

        vector<Rect> temp_rts;
        int rts_size = rts.size();
        for (int i = 0; i < rts_size; i++) {
            Rect rt = rts[i];

            // if not overlap then add
            if (rt.bl_x >= temp.tr_x || rt.tr_x <= temp.bl_x || rt.bl_y >= temp.tr_y || rt.tr_y <= temp.bl_y) {
                temp_rts.emplace_back(rt);
                continue;
            }

            // intersect types
            int bit0 = rt.bl_x < temp.bl_x ? 0 : 1;
            int bit1 = rt.bl_y < temp.bl_y ? 0 : 2;
            int bit2 = rt.tr_x > temp.tr_x ? 0 : 4;
            int bit3 = rt.tr_y > temp.tr_y ? 0 : 8;
            int condition = bit0 + bit1 + bit2 + bit3;
            
            Rect rt_new;
            rt_new.near_criticals = temp.is_critical ? rt.near_criticals + 1 : rt.near_criticals;

            if (condition == 0) {
                // middle (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // four new rects
                // slice along x
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = temp.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // slice along y
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // top
                rt_new.bl_x = temp.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = temp.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // insert the larger area
                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 1) {
                // left middle (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // three new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = temp.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = temp.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 2) {
                // bottom middle (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // three new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = temp.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // top
                rt_new.bl_x = temp.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = temp.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 3) {
                // bottom left (rt.bl_x >= temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y > temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // two new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = temp.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 4) {
                // right middle (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // three new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = temp.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // top
                rt_new.bl_x = temp.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 5) {
                // horizontal middle (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
                // two new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y =  rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
                // bottom
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
                    
            } else if (condition == 6) {
                // bottom right (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // two new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = temp.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 7) {
                // bottom (rt.bl_x >= temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y > temp.tr_y)
                // one new rect
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.tr_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            } else if (condition == 8) {
                // top middle (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // three new rects
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_x = temp.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 9) {
                // top left (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // two new rects
                // top
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = temp.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                //right
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 10) {
                // vertical middle (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
                // two new rects
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.tr_x = rt.tr_x;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            } else if (condition == 11) {
                // left (rt.bl_x >= temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x > temp.tr_x && rt.tr_y <= temp.tr_y)
                // one new rect
                // right
                rt_new.bl_x = temp.tr_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            } else if (condition == 12) {
                // top right (rt.bl_x < temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y <= temp.tr_y)
                vector<Rect> temp1;
                vector<Rect> temp2;
                long long valid_area1 = 0;
                long long valid_area2 = 0;
                // two new rects
                // top
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = temp.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // bottom
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp1.emplace_back(rt_new);
                    valid_area1 += (long long)rt_new.width_x * rt_new.width_y;
                }

                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }
                // right
                rt_new.bl_x = temp.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width) {
                    temp2.emplace_back(rt_new);
                    valid_area2 += (long long)rt_new.width_x * rt_new.width_y;
                }

                if (valid_area1 > valid_area2)
                    temp_rts.insert(temp_rts.end(), temp1.begin(), temp1.end());
                else
                    temp_rts.insert(temp_rts.end(), temp2.begin(), temp2.end());
            } else if (condition == 13) {
                // top (rt.bl_x >= temp.bl_x && rt.bl_y < temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y <= temp.tr_y)
                // one new rect
                // bottom
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = rt.tr_x;
                rt_new.tr_y = temp.bl_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            } else if (condition == 14) {
                // right (rt.bl_x < temp.bl_x && rt.bl_y >= temp.bl_y && rt.tr_x <= temp.tr_x && rt.tr_y <= temp.tr_y)
                // one new rect
                // left
                rt_new.bl_x = rt.bl_x;
                rt_new.bl_y = rt.bl_y;
                rt_new.tr_x = temp.bl_x;
                rt_new.tr_y = rt.tr_y;
                rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
                rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
                if (rt_new.width_x >= actual_min_width && rt_new.width_y >= actual_min_width)
                    temp_rts.emplace_back(rt_new);
            }
        }

        rts.swap(temp_rts);
    }

    sort(rts.begin(), rts.end(), RectSortOrder);
}

void AddMetalFill(Window &w, const Rect rt, const int layer)
{
    Layout metal_fill;
    metal_fill.id = ++total_metals;
    metal_fill.bl_x = rt.bl_x;
    metal_fill.bl_y = rt.bl_y;
    metal_fill.tr_x = rt.tr_x;
    metal_fill.tr_y = rt.tr_y;
    metal_fill.net_id = 0;
    metal_fill.layer = layer;
    metal_fill.type = 3; // Fill
    metal_fill.is_critical = 0;

    int width_x = metal_fill.tr_x - metal_fill.bl_x;
    int width_y = metal_fill.tr_y - metal_fill.bl_y;
    long long metal_fill_area = (long long)width_x * width_y;
    w.area += metal_fill_area;
    w.area_insufficient -= metal_fill_area;

    for (int i = 0; i < 4; i++) {
        QuarterWindow &qw = quarter_windows[layer - 1][w.included_qwindow[i]];
        if (!(metal_fill.bl_x >= qw.tr_x || metal_fill.tr_x <= qw.bl_x || metal_fill.bl_y >= qw.tr_y || metal_fill.tr_y <= qw.bl_y)) {
            int bl_x = metal_fill.bl_x > qw.bl_x ? metal_fill.bl_x : qw.bl_x;
            int bl_y = metal_fill.bl_y > qw.bl_y ? metal_fill.bl_y : qw.bl_y;
            int tr_x = metal_fill.tr_x < qw.tr_x ? metal_fill.tr_x : qw.tr_x;
            int tr_y = metal_fill.tr_y < qw.tr_y ? metal_fill.tr_y : qw.tr_y;
            long long area = (long long)(tr_x - bl_x) * (tr_y - bl_y);

            qw.area += area;
            qw.contribute_metals.emplace_back(metal_fill.id);

            int size = qw.affected_window.size();
            for (int j = 0; j < size; j++) {
                if (qw.affected_window[j] != w.index) {
                    windows[layer - 1][qw.affected_window[j]].area += area;
                    windows[layer - 1][qw.affected_window[j]].area_insufficient -= area;
                }
            }    
        }
    }

    layouts.emplace_back(metal_fill);
    metal_fill_layouts.emplace_back(metal_fill);
}

long long ShrinkMetalFill(Window &w, Rect rt, long long target_area,
                          const int min_width, const int max_width, const int layer)
{
    int x_width = rt.width_x;
    int y_width = rt.width_y;

    int offset = 10;
    int shrink_width;
    while (x_width >= min_width && y_width >= min_width && (long long)x_width * y_width > target_area) {
        // shrink at larger width
        shrink_width = x_width < y_width ? 1 : 0;
        x_width = shrink_width ? x_width : x_width - offset;
        y_width = shrink_width ? y_width - offset : y_width;
    }
    x_width = shrink_width ? x_width : x_width + offset;
    y_width = shrink_width ? y_width + offset : y_width;

    int x_offset = (rt.width_x - x_width) / 2;
    int y_offset = (rt.width_y - y_width) / 2;
    rt.bl_x += x_offset;
    rt.bl_y += y_offset;
    rt.tr_x -= x_offset;
    rt.tr_y -= y_offset;
    rt.width_x = x_width;
    rt.width_y = y_width;
    if (rt.width_x > max_width) {
        rt.tr_x = rt.bl_x + max_width;
        rt.width_x = max_width;
    }
    if (rt.width_y > max_width) {
        rt.tr_y = rt.bl_y + max_width;
        rt.width_y = max_width;
    }

    AddMetalFill(w, rt, layer);
    return (long long)rt.width_x * rt.width_y;
}

long long DivideMetalFill(Window &w, Rect rt, long long target_area,
                          const int min_width, const int max_width, const int min_space, const int layer)
{
    vector<Rect> rts;
    int x_width = rt.width_x;
    int y_width = rt.width_y;
    int max_width_padding = max_width + min_space;

    Rect r;
    r.bl_x = -1;
    r.bl_y = -1;
    r.tr_x = -1;
    r.tr_y = -1;
    r.width_x =  max_width;
    r.width_y =  max_width;
    int x_max_count = x_width / max_width_padding;
    int y_max_count = y_width / max_width_padding;
    // x, y = max width
    for (int y_count = 0; y_count < y_max_count; y_count++) {
        r.bl_y = rt.bl_y + y_count * max_width_padding;
        r.tr_y = r.bl_y + max_width;
        for (int x_count = 0; x_count < x_max_count; x_count++) {
            r.bl_x = rt.bl_x + x_count * max_width_padding;
            r.tr_x = r.bl_x + max_width;
            rts.emplace_back(r);
        }
    }

    int x_width_rest = x_width % max_width_padding;
    if (x_width_rest > max_width)
        x_width_rest = max_width;
    int y_width_rest = y_width % max_width_padding;
    if (y_width_rest > max_width)
        y_width_rest = max_width;

    // larger rect fill first
    if (x_width_rest >= y_width_rest) {
        // y = max width
        if (x_width_rest != 0 && x_width_rest >= min_width) {
            r.bl_x = rt.bl_x + x_max_count * max_width_padding;
            r.tr_x = r.bl_x + x_width_rest;
            r.width_x = x_width_rest;
            r.width_y = max_width;
            for (int y_count = 0; y_count < y_max_count; y_count++) {
                r.bl_y = rt.bl_y + y_count * max_width_padding;
                r.tr_y = r.bl_y + max_width;
                rts.emplace_back(r);
            }
        }
        // x = max width
        if (y_width_rest != 0 && y_width_rest >= min_width) {
            r.bl_y = rt.bl_y + y_max_count * max_width_padding;
            r.tr_y = r.bl_y + y_width_rest;
            r.width_y = y_width_rest;
            r.width_x = max_width;
            for (int x_count = 0; x_count < x_max_count; x_count++) {
                r.bl_x = rt.bl_x + x_count * max_width_padding;
                r.tr_x = r.bl_x + max_width;
                rts.emplace_back(r);
            }
        }
    } else {
        // x = max width
        if (y_width_rest != 0 && y_width_rest >= min_width) {
            r.bl_y = rt.bl_y + y_max_count * max_width_padding;
            r.tr_y = r.bl_y + y_width_rest;
            r.width_y = y_width_rest;
            r.width_x = max_width;
            for (int x_count = 0; x_count < x_max_count; x_count++) {
                r.bl_x = rt.bl_x + x_count * max_width_padding;
                r.tr_x = r.bl_x + max_width;
                rts.emplace_back(r);
            }
        }
        // y = max width
        if (x_width_rest != 0 && x_width_rest >= min_width) {
            r.bl_x = rt.bl_x + x_max_count * max_width_padding;
            r.tr_x = r.bl_x + x_width_rest;
            r.width_x = x_width_rest;
            r.width_y = max_width;
            for (int y_count = 0; y_count < y_max_count; y_count++) {
                r.bl_y = rt.bl_y + y_count * max_width_padding;
                r.tr_y = r.bl_y + max_width;
                rts.emplace_back(r);
            }
        }
    }

    // bottom right
    if (x_width_rest != 0 && x_width_rest >= min_width && y_width_rest != 0 && y_width_rest >= min_width) {
        r.bl_x = rt.bl_x + x_max_count * max_width_padding;
        r.tr_x = r.bl_x + x_width_rest;
        r.bl_y = rt.bl_y + y_max_count * max_width_padding;
        r.tr_y = r.bl_y + y_width_rest;
        r.width_x = x_width_rest;
        r.width_y = y_width_rest;
        rts.emplace_back(r);
    }

    // try to fill no larger than target area
    long long total_fill_area = 0;
    int size = rts.size();
    int i = 0;
    for (i = 0; i < size; i++) {
        long long rect_area = (long long)rts[i].width_x * rts[i].width_y;
        if (total_fill_area + rect_area > target_area)
            break;
        AddMetalFill(w, rts[i], layer);
        total_fill_area += rect_area;
    }
    // if break, try to shrink
    if (i != size)
        total_fill_area += ShrinkMetalFill(w, rts[i], target_area - total_fill_area, min_width, max_width, layer);

    return total_fill_area;
}

long long MinMetalFill(Window &w, Rect rt, const int min_width, const int layer)
{
    int x_offset = (rt.width_x - min_width) / 2;
    int y_offset = (rt.width_y - min_width) / 2;
    rt.bl_x += x_offset;
    rt.bl_y += y_offset;
    rt.tr_x -= x_offset;
    rt.tr_y -= y_offset;
    rt.width_x = min_width;
    rt.width_y = min_width;

    AddMetalFill(w, rt, layer);
    return (long long)rt.width_x * rt.width_y;
}

long long UpdateMetalFill(Window &w, const Rect rt, const int layer)
{
    AddMetalFill(w, rt, layer);
    return (long long)rt.width_x * rt.width_y;
}

void FillMetal()
{
    // window area;
    long long w_area = (long long)window_size * window_size;

    for (int layer = 1; layer <= total_layers; layer++) {
        vector<Window> &ws = windows[layer - 1];
        vector<QuarterWindow> &qws = quarter_windows[layer - 1];
        const Rule &r = rules[layer - 1];

        // half of min_space
        int half_min_space = (r.min_space + 1) / 2;
        // min quarter window area
        long long min_area = (long long)stride * stride * r.min_density;
        // max, min metal fill area
        long long min_metal_fill = (long long)r.min_width * r.min_width;
        long long max_metal_fill = (long long)r.max_fill_width * r.max_fill_width;

        int max_windows = window_x * window_y;
        while (1) {
            int min_insuff_idx = -1;
            long long min_insuff = w_area;
            for (int i = 0; i < max_windows; i++) {
                if (ws[i].area_insufficient > 0 && ws[i].area_insufficient < min_insuff) {
                    min_insuff_idx = i;
                    min_insuff = ws[i].area_insufficient;
                }
            }

            // no more insufficient window
            if (min_insuff_idx == -1)
                break;

            Window &w = ws[min_insuff_idx];
            long long target_area = min_insuff;

            vector<int> contribute_metals;
            int top_left = w.included_qwindow[0];
            int top_right = w.included_qwindow[1];
            int bottom_left = w.included_qwindow[2];
            int bottom_right = w.included_qwindow[3];
            // left
            if (top_left % qwindow_y != 0)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[top_left - 1].contribute_metals.begin(),
                                         qws[top_left - 1].contribute_metals.end());
            if (bottom_left % qwindow_y != 0)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[bottom_left - 1].contribute_metals.begin(),
                                         qws[bottom_left - 1].contribute_metals.end());
            // top
            if (top_left / qwindow_y != 0)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[top_left - qwindow_y].contribute_metals.begin(),
                                         qws[top_left - qwindow_y].contribute_metals.end());
            if (top_right / qwindow_y != 0)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[top_right - qwindow_y].contribute_metals.begin(),
                                         qws[top_right - qwindow_y].contribute_metals.end());
            // top left
            if (top_left % qwindow_y != 0 && top_left / qwindow_y != 0)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[top_left - qwindow_y - 1].contribute_metals.begin(),
                                         qws[top_left - qwindow_y - 1].contribute_metals.end());
            // right
            if (top_right % qwindow_y != qwindow_y - 1)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[top_right + 1].contribute_metals.begin(),
                                         qws[top_right + 1].contribute_metals.end());
            if (bottom_right % qwindow_y != qwindow_y - 1)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[bottom_right + 1].contribute_metals.begin(),
                                         qws[bottom_right + 1].contribute_metals.end());
            // top right
            if (top_right % qwindow_y != qwindow_y - 1 && top_right / qwindow_y != 0)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[top_right - qwindow_y + 1].contribute_metals.begin(),
                                         qws[top_right - qwindow_y + 1].contribute_metals.end());
            // bottom
            if (bottom_left / qwindow_y != qwindow_x - 1)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[bottom_left + qwindow_y].contribute_metals.begin(),
                                         qws[bottom_left + qwindow_y].contribute_metals.end());
            if (bottom_right / qwindow_y != qwindow_x - 1)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[bottom_right + qwindow_y].contribute_metals.begin(),
                                         qws[bottom_right + qwindow_y].contribute_metals.end());
            // bottom left
            if (bottom_left % qwindow_y != 0 && bottom_left / qwindow_y != qwindow_x - 1)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[bottom_left + qwindow_y - 1].contribute_metals.begin(),
                                         qws[bottom_left + qwindow_y - 1].contribute_metals.end());
            // bottom right
            if (bottom_right % qwindow_y != qwindow_y - 1 && bottom_right / qwindow_y != qwindow_x - 1)
                contribute_metals.insert(contribute_metals.end(),
                                         qws[bottom_right + qwindow_y + 1].contribute_metals.begin(),
                                         qws[bottom_right + qwindow_y + 1].contribute_metals.end());
            // original
            contribute_metals.insert(contribute_metals.end(),
                                     qws[top_left].contribute_metals.begin(),
                                     qws[top_left].contribute_metals.end());
            contribute_metals.insert(contribute_metals.end(),
                                     qws[top_right].contribute_metals.begin(),
                                     qws[top_right].contribute_metals.end());
            contribute_metals.insert(contribute_metals.end(),
                                     qws[bottom_left].contribute_metals.begin(),
                                     qws[bottom_left].contribute_metals.end());
            contribute_metals.insert(contribute_metals.end(),
                                     qws[bottom_right].contribute_metals.begin(),
                                     qws[bottom_right].contribute_metals.end());

            vector<Rect> rts;
            Rect w_rect;
            w_rect.bl_x = w.bl_x - half_min_space;
            w_rect.bl_y = w.bl_y - half_min_space;
            w_rect.tr_x = w.tr_x + half_min_space;
            w_rect.tr_y = w.tr_y + half_min_space;
            w_rect.width_x = w_rect.tr_x - w_rect.bl_x;
            w_rect.width_y = w_rect.tr_y - w_rect.bl_y;
            w_rect.near_criticals = 0;
            rts.emplace_back(w_rect);

            FindSpace(rts, set<int>(contribute_metals.begin(), contribute_metals.end()), r.min_width, half_min_space);

            int size = rts.size();
            for (int i = 0; i < size; i++) {
                Rect metal_fill = rts[i];
                metal_fill.bl_x += half_min_space;
                metal_fill.bl_y += half_min_space;
                metal_fill.tr_x -= half_min_space;
                metal_fill.tr_y -= half_min_space;
                metal_fill.width_x = metal_fill.tr_x -  metal_fill.bl_x;
                metal_fill.width_y = metal_fill.tr_y -  metal_fill.bl_y;
                long long metal_fill_area = (long long)metal_fill.width_x * metal_fill.width_y;

                if (target_area < min_metal_fill)
                    target_area -= MinMetalFill(w, metal_fill, r.min_width, layer);
                else
                    if (metal_fill.width_x <= r.max_fill_width && metal_fill.width_y <= r.max_fill_width)
                        if (target_area >= metal_fill_area)
                            target_area -= UpdateMetalFill(w, metal_fill, layer);
                        else
                            target_area -= ShrinkMetalFill(w, metal_fill, target_area,
                                                           r.min_width, r.max_fill_width, layer);
                    else
                        target_area -= DivideMetalFill(w, metal_fill, target_area,
                                                       r.min_width, r.max_fill_width, r.min_space, layer);

                if (target_area <= 0)
                    break;
            }

            if (target_area > 0) {
                printf("[Error] target area remaining after metal fill\n");
                exit(1);
            }
        }
    }
}

void OutputFill()
{
    ofstream file(path + output_file);

    int size = metal_fill_layouts.size();
    for (int i = 0; i < size; i++) {
        Layout &temp = metal_fill_layouts[i];
        file << temp.id << " " << temp.bl_x + cb.bl_x << " " << temp.bl_y + cb.bl_y << " "
             << temp.tr_x + cb.bl_x << " " << temp.tr_y + cb.bl_y << " "
             << temp.net_id << " " << temp.layer << " Fill\n";
    }

    file.close();
}

/* output layout in layer order */
void OutputAll()
{
    ofstream file(path + output_file);

    file << cb.bl_x << " " << cb.bl_y << " " << cb.tr_x << " " << cb.tr_y << "; chip boundary\n";

    int size = layouts.size();
    for (int layer = 1; layer <= total_layers; layer++) {
        for (int i = 1; i < size; i++) {
            Layout &temp = layouts[i];
            if (temp.layer == layer) {
                file << temp.id << " " << temp.bl_x + cb.bl_x << " " << temp.bl_y + cb.bl_y << " "
                     << temp.tr_x + cb.bl_x << " " << temp.tr_y + cb.bl_y << " "
                     << temp.net_id << " " << temp.layer;
                if (temp.type == 1)
                    file << " Normal\n";
                else if (temp.type == 3)
                    file << " Fill\n";
            }
        }
    }

    file.close();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: ./<executable> <config file>\n");
        exit(1);
    }
    config_file = argv[1];
    path = config_file.substr(0, config_file.find_last_of("/") + 1);

    ReadConfig();
    ReadCircuit();
    ReadProcess();
    ReadRule();

    AnalyzeDensity();
    
    // initialize cap table (symmetric lower traingular matrix)
    // long long total_map_size = total_metals * (total_metals + 1) / 2;
    // for (long long i = 0; i < total_map_size; i++)
    //     cap[i] = -1;
    // CalculateAreaCapacitance();
    // CalculateLateralCapacitance();
    // CalculateFringeCapacitance();

#ifndef CHECKER_MODE
    FillMetal();
    #ifdef OUTPUT_ALL
        OutputAll();
    #else
        OutputFill();
    #endif
#endif

    return 0;
}
