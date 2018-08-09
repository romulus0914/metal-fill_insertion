#include <algorithm>

#include "problem_c.h"

using namespace std;

/*
    type: ["area""lateral""fringe"]
    layer1: [0-9]
    layer2: [1-9]
    param1: parameter to look up in tables, (s for "area", d for "lateral" and "fringe")
    param2: parameter to multiply, (s for "area", l for "lateral" and "fringe")
*/
double CalculateCouplingCapacitance(string type,int layer1, int layer2, double param1, double param2)
{
    if (type == "area") {
        const AreaTable &atbl = area_table_map[area_tables[layer1 * total_layers + layer2 - 1]];
        int size = atbl.s.size();
        for (int i = 0; i < size; i++) {
            if (param1 < atbl.s[i]) {
                if (i == 0)
                    return (atbl.a[0] * atbl.s[0] + atbl.b[0]) * atbl.s[0] * (param1 / atbl.s[0]);
                else
                    return (atbl.a[i - 1] * param1 + atbl.b[i - 1]) * param1;
            }
        }
        int last = size - 1;
        return (atbl.a[last] * atbl.s[last] + atbl.b[last]) * atbl.s[last] * (param1 / atbl.s[last]);
    } else if (type == "lateral") {
        const FringeTable &ltbl = fringe_table_map[fringe_tables[layer1 * total_layers + layer1 - 1]]; // layer1 == layer2
        int size = ltbl.d.size();
        for (int i = 0; i < size; i++) {
            if (param1 < ltbl.d[i]) {
                if (i == 0)
                    return 0.0;
                else
                    return (ltbl.a[i - 1] * param1 + ltbl.b[i - 1]) * param2;
            }
        }
        return 0.0;
    } else if (type == "fringe") {
        const FringeTable &ftbl1 = fringe_table_map[fringe_tables[layer1 * total_layers + layer2 - 1]];
        const FringeTable &ftbl2 = fringe_table_map[fringe_tables[layer2 * total_layers + layer1 - 1]];
        double coupling_cap = 0.0;
        int size = ftbl1.d.size();
        for (int i = 0; i < size; i++)
            if (i != 0 && param1 < ftbl1.d[i])
                coupling_cap = (ftbl1.a[i - 1] * param1 + ftbl1.b[i - 1]) * param2;
        size = ftbl2.d.size();
        for (int i = 0; i < size; i++)
            if (i != 0 && param1 < ftbl2.d[i])
                return coupling_cap + (ftbl2.a[i - 1] * param1 + ftbl2.b[i - 1]) * param2;
        return coupling_cap;
    } else {
        printf("[Error] undefined coupling capcitance type\n");
        exit(1);
    }
}

/* transform two ids to symmetric, lower triangular matrix index (0 based) */
long long IdToIndex(int id1, int id2)
{
    if (id1 == 0 && id2 == 0)
        return 0;
    else if (id1 <= id2)
        return (1 + id2) * id2 / 2 + id1;
    else
        return (1 + id1) * id1 / 2 + id2;
}

long long AreaShielding(vector<Rect> &rts, const Layout &temp)
{
    vector<Rect> temp_rts;
    // overlap area
    long long area = 0;

    int size = rts.size();
    for (int i = 0; i < size; i++) {
        Rect &rt = rts[i];
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
        if (condition == 0) {
            // middle
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // left
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = temp.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // right
            rt_new.bl_x = temp.tr_x;
            rt_new.tr_x = rt.tr_x;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 1) {
            // left middle
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // right
            rt_new.bl_x = temp.tr_x;
            rt_new.bl_y = temp.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = temp.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 2) {
            // bottom middle
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // left
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = temp.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // right
            rt_new.bl_x = temp.tr_x;
            rt_new.tr_x = rt.tr_x;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 3) {
            // bottom left
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_x = temp.tr_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = temp.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 4) {
            // right middle
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // left
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = temp.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 5) {
            // horizontal middle
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y =  rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
                
        } else if (condition == 6) {
            // bottom right
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
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = temp.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 7) {
            // bottom
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.tr_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 8) {
            // top middle
            // left
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // right
            rt_new.bl_x = temp.tr_x;
            rt_new.tr_x = rt.tr_x;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 9) {
            // top left
            // top
            rt_new.bl_x = temp.tr_x;
            rt_new.bl_y = temp.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 10) {
            // vertical middle
            // left
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // right
            rt_new.bl_x = temp.tr_x;
            rt_new.tr_x = rt.tr_x;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 11) {
            // left
            // right
            rt_new.bl_x = temp.tr_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 12) {
            // top right
            // top
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = temp.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
            // bottom
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 13) {
            // top
            // bottom
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = rt.tr_x;
            rt_new.tr_y = temp.bl_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        } else if (condition == 14) {
            // right
            // one new rect
            // left
            rt_new.bl_x = rt.bl_x;
            rt_new.bl_y = rt.bl_y;
            rt_new.tr_x = temp.bl_x;
            rt_new.tr_y = rt.tr_y;
            rt_new.width_x = rt_new.tr_x - rt_new.bl_x;
            rt_new.width_y = rt_new.tr_y - rt_new.bl_y;
            temp_rts.emplace_back(rt_new);
        }

        // overlap area
        int bl_x = temp.bl_x > rt.bl_x ? temp.bl_x : rt.bl_x;
        int bl_y = temp.bl_y > rt.bl_y ? temp.bl_y : rt.bl_y;
        int tr_x = temp.tr_x < rt.tr_x ? temp.tr_x : rt.tr_x;
        int tr_y = temp.tr_y < rt.tr_y ? temp.tr_y : rt.tr_y;
        area += (long long)(tr_x - bl_x) * (tr_y - bl_y);
    }

    rts.swap(temp_rts);
    return area;
}

void CalculateShieldingAreaCap(const Layout &cur_metal, const Layout &temp, long long area)
{
    // area capacitance
    double area_cap = 0.0;
    // metals with same net id do not have coupling capacitance (but still shield?)
    if (cur_metal.net_id != temp.net_id && area != 0)
        area_cap = CalculateCouplingCapacitance("area", cur_metal.layer, temp.layer, area, area);
    cap[IdToIndex(cur_metal.id - 1, temp.id - 1)] = area_cap;
}

struct area_order {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].layer > layouts[metal2].layer;
    }
} AreaOrder;

void CalculateAreaCapacitance()
{
    for (int current_metal = 1; current_metal <= total_metals; current_metal++) {
        const Layout &cur_metal = layouts[current_metal];
        // quarter window indexn
        int x_from = cur_metal.bl_x / stride;
        int x_to = (cur_metal.tr_x - 1) / stride;
        int y_from = cur_metal.bl_y / stride;
        int y_to = (cur_metal.tr_y - 1) / stride;

        // candidate lateral metals
        set<int> candidate_metals;
        // search quarter window for candidate metals in below layers
        for (int layer = cur_metal.layer - 1; layer > 0; layer--) {
            for (int x = x_from; x <= x_to; x++) {
                for (int y = y_from; y <= y_to; y++) {
                    const vector<int> &qwindow_metals = quarter_windows[layer - 1][x * qwindow_y + y].contribute_metals;
                    // no itself and no area cap would calculate twice
                    candidate_metals.insert(qwindow_metals.begin(), qwindow_metals.end());
                }
            }
        }

        // set can't sort
        vector<int> cand_metals(candidate_metals.begin(), candidate_metals.end());
        // the nearer of layer to cur_metal, the sooner the metal should be calculated
        sort(cand_metals.begin(), cand_metals.end(), AreaOrder);
        
        vector<Rect> rts;
        Rect rt;
        long long area = 0;
        
        rt.bl_x = cur_metal.bl_x;
        rt.bl_y = cur_metal.bl_y;
        rt.tr_x = cur_metal.tr_x;
        rt.tr_y = cur_metal.tr_y;
        rt.width_x = rt.tr_x - rt.bl_x;
        rt.width_y = rt.tr_y - rt.bl_y;
        rts.emplace_back(rt);
        if (cur_metal.layer != 1) {
            for (vector<int>::iterator it = cand_metals.begin(); it != cand_metals.end(); it++) {
                const Layout &temp = layouts[*it];

                // metals that do not intersect
                if (temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x ||
                    temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y)
                    continue;

                area = AreaShielding(rts, temp);
                CalculateShieldingAreaCap(cur_metal, temp, area);
            }
        }

        // area cap to ground(layer 0)
        double area_cap = 0.0;
        area = 0;
        if (!rts.empty()) {
            int size = rts.size();
            for (int i = 0; i < size; i++) {
                Rect &rt = rts[i];
                area += (long long)rt.width_x * rt.width_y;
            }
            area_cap = CalculateCouplingCapacitance("area", 0, cur_metal.layer, area, area);
        }
        // store coupling cap to ground at <cur_metal.id, cur_metal.id>
        cap[IdToIndex(cur_metal.id - 1, cur_metal.id - 1)] = area_cap;
    }
}

int Shielding(vector<Line> &ls, const int left, const int right)
{
    vector<Line> temp_ls;
    // overlap length
    int lateral_length = 0;

    int size = ls.size();
    for (int i = 0; i < size; i++) {
        Line &l = ls[i];
        // if no overlap then add
        if (right <= l.left || left >= l.right)
            temp_ls.emplace_back(l);

        // default overlap
        int overlap_l = l.left;
        int overlap_r = l.right;

        Line l_new;
        if (left > l.left) {
            l_new.left = l.left;
            l_new.right = left;
            temp_ls.emplace_back(l_new);

            overlap_l = left;
        }
        if (right < l.right) {
            l_new.left = right;
            l_new.right = l.right;
            temp_ls.emplace_back(l_new);

            overlap_r = right;
        }

        lateral_length += overlap_r - overlap_l;
    }

    ls.swap(temp_ls);
    return lateral_length;
}

void CalculateShieldingLateralCap(const Layout &cur_metal, const Layout &temp, const int distance, const int length)
{
    // lateral capacitance
    double lateral_cap = 0.0;
    // metals with same net id do not have coupling capacitance (but still shield?)
    if (cur_metal.net_id != temp.net_id && length != 0)
        lateral_cap = CalculateCouplingCapacitance("lateral", cur_metal.layer, temp.layer, distance, length);
    cap[IdToIndex(cur_metal.id - 1, temp.id - 1)] = lateral_cap;
}

struct lateral_order_up {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].bl_y < layouts[metal2].bl_y;
    }
} LateralOrderUp;

struct lateral_order_down {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].tr_y > layouts[metal2].tr_y;
    }
} LateralOrderDown;

struct lateral_order_left {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].tr_x > layouts[metal2].tr_x;
    }
} LateralOrderLeft;

struct lateral_order_right {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].bl_x < layouts[metal2].bl_x;
    }
} LateralOrderRight;

void CalculateLateralCapacitance()
{
    for (int current_metal = 1; current_metal <= total_metals; current_metal++) {
        const Layout &cur_metal = layouts[current_metal];
        // quarter window indexn
        int x_from = cur_metal.bl_x / stride;
        int x_to = (cur_metal.tr_x - 1) / stride;
        int y_from = cur_metal.bl_y / stride;
        int y_to = (cur_metal.tr_y - 1) / stride;

        // candidate lateral metals
        set<int> candidate_metals;
        // search quarter window for candidate metals, only need to consider the same row or col
        // same row
        for (int x = 0; x < qwindow_x; x++) {
            for (int y = y_from; y <= y_to; y++) {
                const vector<int> &qwindow_metals = quarter_windows[cur_metal.layer - 1][x * qwindow_y + y].contribute_metals;
                int size = qwindow_metals.size();
                for (int i = 0; i < size; i++) {
                    int metal_id = qwindow_metals[i];
                    // calculate already then skip
                    if (cap[IdToIndex(current_metal - 1, metal_id - 1)] != -1)
                        continue;
                    candidate_metals.insert(metal_id);
                }
            }
        }
        // same col
        for (int y = 0; y < qwindow_y; y++) {
            for (int x = x_from; x <= x_to; x++) {
                const vector<int> &qwindow_metals = quarter_windows[cur_metal.layer - 1][x * qwindow_y + y].contribute_metals;
                int size = qwindow_metals.size();
                for (int i = 0; i < size; i++) {
                    int metal_id = qwindow_metals[i];
                    // calculate already then skip
                    if (cap[IdToIndex(current_metal - 1, metal_id - 1)] != -1)
                        continue;
                    candidate_metals.insert(metal_id);
                }
            }
        }
        // remove itself
        candidate_metals.erase(current_metal);

        // determine remaining metals belong to which direction
        vector<int> up, down, left, right;
        for (set<int>::iterator it = candidate_metals.begin(); it != candidate_metals.end(); it++) {
            int metal_id = *it;
            const Layout &temp = layouts[metal_id];

            // determine if they can't see each other then skip (haven't consider shielding)
            if ((temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x) &&
                (temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
                continue;

            // determine in which direction
            if (temp.bl_y >=  cur_metal.tr_y)
                up.emplace_back(metal_id);
            else if (temp.tr_y <= cur_metal.bl_y)
                down.emplace_back(metal_id);
            else if (temp.tr_x <= cur_metal.bl_x)
                left.emplace_back(metal_id);
            else if (temp.bl_x >= cur_metal.tr_x)
                right.emplace_back(metal_id);
            else
                /* shouldn't overlap, testcase problems, via metal */
                continue;
        }
        set<int>().swap(candidate_metals); // not sure if this works for set as vector to free memory

        // the nearer to the cur_metal, the sooner the metal should be calculated
        sort(up.begin(), up.end(), LateralOrderUp);
        sort(down.begin(), down.end(), LateralOrderDown);
        sort(left.begin(), left.end(), LateralOrderLeft);
        sort(right.begin(), right.end(), LateralOrderRight);

        vector<Line> ls;
        Line cur_metal_line;
        int lateral_length = 0;

        // up
        cur_metal_line.left = cur_metal.bl_x;
        cur_metal_line.right = cur_metal.tr_x;
        ls.emplace_back(cur_metal_line);
        int size = up.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[up[i]];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;

            lateral_length = Shielding(ls, temp.bl_x, temp.tr_x);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);

        // down
        cur_metal_line.left = cur_metal.bl_x;
        cur_metal_line.right = cur_metal.tr_x;
        ls.emplace_back(cur_metal_line);
        size = down.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[down[i]];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;

            lateral_length = Shielding(ls, temp.bl_x, temp.tr_x);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);

        // left
        cur_metal_line.left = cur_metal.bl_y;
        cur_metal_line.right = cur_metal.tr_y;
        ls.emplace_back(cur_metal_line);
        size = left.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[left[i]];
            // distance between two metals
            int distance = cur_metal.bl_x - temp.tr_x;

            lateral_length = Shielding(ls, temp.bl_y, temp.tr_y);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);

        // right
        cur_metal_line.left = cur_metal.bl_y;
        cur_metal_line.right = cur_metal.tr_y;
        ls.emplace_back(cur_metal_line);
        size = right.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[right[i]];
            // distance between two metals
            int distance = temp.bl_x - cur_metal.tr_x;

            lateral_length = Shielding(ls, temp.bl_y, temp.tr_y);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);
    }
}

void CalculateShieldingFringeCap(const Layout &cur_metal, const Layout &temp, const int distance, const int length)
{
    // no fringe cap between metals that are in same layer or projection overlapping, but still cause shielding
    if ((cur_metal.layer == temp.layer) ||
        !(temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x ||
          temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
        return;

    // fringe capacitance
    double fringe_cap = 0.0;
    // metals with same net id do not have coupling capacitance (but still shield?)
    if (cur_metal.net_id != temp.net_id && length != 0)
        fringe_cap = CalculateCouplingCapacitance("fringe", cur_metal.layer, temp.layer, distance, length);
    cap[IdToIndex(cur_metal.id - 1, temp.id - 1)] = fringe_cap;
}

struct fringe_order_up {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].bl_y < layouts[metal2].bl_y;
    }
} FringeOrderUp;

struct fringe_order_down {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].tr_y > layouts[metal2].tr_y;
    }
} FringeOrderDown;

struct fringe_order_left {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].tr_x > layouts[metal2].tr_x;
    }
} FringeOrderLeft;

struct fringe_order_right {
    bool operator()(int metal1, int metal2) const {
        return layouts[metal1].bl_x < layouts[metal2].bl_x;
    }
} FringeOrderRight;

/* should be similar to lateral, but consider overlapping coordinate in different layers */
void CalculateFringeCapacitance()
{
    for (int current_metal = 1; current_metal <= total_metals; current_metal++) {
        const Layout &cur_metal = layouts[current_metal];
        // the highest layer no more fringe cap to calculate
        if (cur_metal.layer == total_layers)
            break;
        // quarter window index
        int x_from = cur_metal.bl_x / stride;
        int x_to = (cur_metal.tr_x - 1) / stride;
        int y_from = cur_metal.bl_y / stride;
        int y_to = (cur_metal.tr_y - 1) / stride;

        // candidate fringe metals
        set<int> candidate_metals;
        for (int layer = cur_metal.layer; layer <= total_layers; layer++) {
            // search quarter window for candidate metals,
            //     only need to consider the same row or col non matter in which layer
            // same row
            for (int x = 0; x < qwindow_x; x++) {
                for (int y = y_from; y <= y_to; y++) {
                    const vector<int> &qwindow_metals = quarter_windows[layer - 1][x * qwindow_y + y].contribute_metals;
                    candidate_metals.insert(qwindow_metals.begin(), qwindow_metals.end());
                }
            }
            // same col
            for (int y = 0; y < qwindow_y; y++) {
                for (int x = x_from; x <= x_to; x++) {
                    const vector<int> &qwindow_metals = quarter_windows[layer - 1][x * qwindow_y + y].contribute_metals;
                    candidate_metals.insert(qwindow_metals.begin(), qwindow_metals.end());
                }
            }
        }
        candidate_metals.erase(current_metal);

        // determine remaining metals belong to which direction
        vector<int> up, down, left, right;
        for (set<int>::iterator it = candidate_metals.begin(); it != candidate_metals.end(); it++) {
            int metal_id = *it;
            const Layout &temp = layouts[metal_id];

            // determine if they can't see each other then skip (haven't consider shielding)
            if ((temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x) &&
                (temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
                continue;

            // determine in which direction
            // one metal may belong to more than one direction because projection ovelapping from different layers
            if (temp.tr_y > cur_metal.tr_y)
                up.emplace_back(metal_id);
            if (temp.bl_y < cur_metal.bl_y)
                down.emplace_back(metal_id);
            if (temp.bl_x < cur_metal.bl_x)
                left.emplace_back(metal_id);
            if (temp.tr_x > cur_metal.tr_x)
                right.emplace_back(metal_id);
        }
        set<int>().swap(candidate_metals); // not sure if this works for set as vector to free memory

        // the nearer to the cur_metal, the sooner the metal should be calculated
        // consider the projection overlapping metals the same as others, their orders don't matter
        //     just make sure they are in front of other metals 
        sort(up.begin(), up.end(), FringeOrderUp);
        sort(down.begin(), down.end(), FringeOrderDown);
        sort(left.begin(), left.end(), FringeOrderLeft);
        sort(right.begin(), right.end(), FringeOrderRight);

        vector<Line> ls;
        Line cur_metal_line;
        int lateral_length = 0;

        // up
        cur_metal_line.left = cur_metal.bl_x;
        cur_metal_line.right = cur_metal.tr_x;
        ls.emplace_back(cur_metal_line);
        int size = up.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[up[i]];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;

            lateral_length = Shielding(ls, temp.bl_x, temp.tr_x);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);

        // down
        cur_metal_line.left = cur_metal.bl_x;
        cur_metal_line.right = cur_metal.tr_x;
        ls.emplace_back(cur_metal_line);
        size = down.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[down[i]];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;

            lateral_length = Shielding(ls, temp.bl_x, temp.tr_x);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);

        // left
        cur_metal_line.left = cur_metal.bl_y;
        cur_metal_line.right = cur_metal.tr_y;
        ls.emplace_back(cur_metal_line);
        size = left.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[left[i]];
            // distance between two metals
            int distance = cur_metal.bl_x - temp.tr_x;

            lateral_length = Shielding(ls, temp.bl_y, temp.tr_y);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);

        // right
        cur_metal_line.left = cur_metal.bl_y;
        cur_metal_line.right = cur_metal.tr_y;
        ls.emplace_back(cur_metal_line);
        size = right.size();
        for (int i = 0; i < size; i++) {
            const Layout &temp = layouts[right[i]];
            // distance between two metals
            int distance = temp.bl_x - cur_metal.tr_x;

            lateral_length = Shielding(ls, temp.bl_y, temp.tr_y);
            CalculateShieldingLateralCap(cur_metal, temp, distance, lateral_length);

            if (ls.empty())
                break;
        }
        vector<Line>().swap(ls);
    }
}
