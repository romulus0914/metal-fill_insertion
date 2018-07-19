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
    }
    else if (type == "lateral") {
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
    }
    else if (type == "fringe") {
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
    }
    else {
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

long long CalculateShieldingAreaCap(vector<int> &area, const Layout &cur_metal, const Layout &temp,
                               int bl_x, int bl_y, int tr_x, int tr_y)
{
    long long actual_area = 0;
    
    int x_width = cur_metal.tr_x - cur_metal.bl_x;
    for (int y = bl_y; y < tr_y; y++) {
        for (int x = bl_x; x < tr_x; x++) {
            if (area[y * x_width + x] == 0) {
                acutal_area++;
                area[y * x_width + x] = 1;
            }
        }
    }
    
    // area capacitance
    double area_cap = 0.0;
    if (actual_area != 0)
        area_cap = CalculateCouplingCapacitance("area", cur_metal.layer, temp.layer, actual_area, actual_area);
    cap[IdToIndex(cur_metal.id - 1, temp.id - 1)] = area_cap;
    
    return acutal_area;
}

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

        // area remained without shielding
        long long remaining_area = (cur_metal.tr_x - cur_metal.bl_x) * (cur_metal.tr_y - cur_metal.bl_y);
        
        if (cur_metal.layer != 1) {
            vector<int> area(remaining_area, 0);
            for (int metal_id : candidate_metals) {
                const Layout &temp = layouts[metal_id];
                if (temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x ||
                    temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y)
                    continue;
                
                // intersect coordinate
                int bl_x = cur_metal.bl_x <= temp.bl_x ? temp.bl_x - cur_metal.bl_x : cur_metal.bl_x - cur_metal.bl_x;
                int bl_y = cur_metal.bl_y <= temp.bl_y ? temp.bl_y - cur_metal.bl_y : cur_metal.bl_y - cur_metal.bl_y;
                int tr_x = cur_metal.tr_x >= temp.tr_x ? temp.tr_x - cur_metal.bl_x : cur_metal.tr_x - cur_metal.bl_x;
                int tr_y = cur_metal.tr_y >= temp.tr_y ? temp.tr_y - cur_metal.bl_y : cur_metal.tr_y - cur_metal.bl_y;
                
                remaining_area -= CalculateShieldingAreaCap(area, cur_metal, temp, bl_x, bl_y, tr_x, tr_y);
                if (remaining_area == 0)
                    break;
            }
        }

        // area cap to ground(layer 0)
        double area_cap = 0.0;
        if (remaining_area != 0)
            area_cap = CalculateCouplingCapacitance("area", cur_metal.layer, 0, remaining_area, remaining_area);
        cap[IdToIndex(cur_metal.id - 1, cur_metal.id - 1)] = area_cap;
    }
}

int CalculateShieldingLateralCap(vector<int> &edge, const Layout &cur_metal, const Layout &temp,
                                 int distance, int start, int end)
{
    int length = 0;
    for (int i = start; i < end; i++) {
        if (edge[i] == 0) {
            length++;
            edge[i] = 1;
        }
    }
    
    // lateral capacitance
    double lateral_cap = 0.0;
    if (length != 0)
        lateral_cap = CalculateCouplingCapacitance("lateral", cur_metal.layer, temp.layer, distance, length);
    cap[IdToIndex(cur_metal.id - 1, temp.id - 1)] = lateral_cap;

    return length;
}

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
                for (int metal_id : qwindow_metals) {
                    // skip itself
                    if (metal_id == current_metal)
                        continue;
                    // calculate already then skip
                    if (cap[IdToIndex(current_metal - 1, metal_id - 1)] != 0)
                        continue;
                    candidate_metals.insert(metal_id);
                }
            }
        }
        // same col
        for (int y = 0; y < qwindow_y; y++) {
            for (int x = x_from; x <= x_to; x++) {
                const vector<int> &qwindow_metals = quarter_windows[cur_metal.layer - 1][x * qwindow_y + y].contribute_metals;
                for (int metal_id : qwindow_metals) {
                    // skip itself
                    if (metal_id == current_metal)
                        continue;
                    // calculate already then skip
                    if (cap[IdToIndex(current_metal - 1, metal_id - 1)] != 0)
                        continue;
                    candidate_metals.insert(metal_id);
                }
            }
        }

        // determine remaining metals belong to which direction
        vector<int> up, down, left, right;
        for (int metal_id : candidate_metals) {
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
            else {
                /* shouldn't overlap, testcase problems, via metal */
                continue;
            }
        }
        set<int>().swap(candidate_metals); // not sure if this works for set as vector to free memory

        // the nearer to the cur_metal, the sooner the metal should be evaluate
        sort(up.begin(), up.end(), [](const int &i, const int &j) {
            return layouts[i].bl_y < layouts[j].bl_y;
        });
        sort(down.begin(), down.end(), [](const int &i, const int &j) {
            return layouts[i].tr_y > layouts[j].tr_y;
        });
        sort(left.begin(), left.end(), [](const int &i, const int &j) {
            return layouts[i].tr_x > layouts[j].tr_x;
        });
        sort(right.begin(), right.end(), [](const int &i, const int &j) {
            return layouts[i].bl_x < layouts[j].bl_x;
        });

        // up
        int width = cur_metal.tr_x - cur_metal.bl_x;
        vector<int> edge(width, 0);
        // remaining lateral edge
        int remaining_length = width;
        for (int metal_id : up) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;
            // raw intersect length (without shielding)
            int x_start = cur_metal.bl_x > temp.bl_x ? cur_metal.bl_x - cur_metal.bl_x : temp.bl_x - cur_metal.bl_x;
            int x_end = cur_metal.tr_x < temp.tr_x ? cur_metal.tr_x - cur_metal.bl_x : temp.tr_x - cur_metal.bl_x;

            remaining_length -= CalculateShieldingLateralCap(edge, cur_metal, temp, distance, x_start, x_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // down
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : down) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;
            // raw intersect length (without shielding)
            int x_start = cur_metal.bl_x > temp.bl_x ? cur_metal.bl_x - cur_metal.bl_x : temp.bl_x - cur_metal.bl_x;
            int x_end = cur_metal.tr_x < temp.tr_x ? cur_metal.tr_x - cur_metal.bl_x : temp.tr_x - cur_metal.bl_x;

            CalculateShieldingLateralCap(edge, cur_metal, temp, distance, x_start, x_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // left
        width = cur_metal.tr_y - cur_metal.bl_y;
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : left) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = cur_metal.bl_x - temp.tr_x;
            // raw intersect length (without shielding)
            int y_start = cur_metal.bl_y > temp.bl_y ? cur_metal.bl_y - cur_metal.bl_y : temp.bl_y - cur_metal.bl_y;
            int y_end = cur_metal.tr_y < temp.tr_y ? cur_metal.tr_y - cur_metal.bl_y : temp.tr_y - cur_metal.bl_y;

            CalculateShieldingLateralCap(edge, cur_metal, temp, distance, y_start, y_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // right
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : right) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_x - cur_metal.tr_x;
            // raw intersect length (without shielding)
            int y_start = cur_metal.bl_y > temp.bl_y ? cur_metal.bl_y - cur_metal.bl_y : temp.bl_y - cur_metal.bl_y;
            int y_end = cur_metal.tr_y < temp.tr_y ? cur_metal.tr_y - cur_metal.bl_y : temp.tr_y - cur_metal.bl_y;

            CalculateShieldingLateralCap(edge, cur_metal, temp, distance, y_start, y_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating lateral capcitance\n");
            //     printf("[Error] error in calculating lateral edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        vector<int>().swap(edge);
    }
}

int CalculateShieldingFringeCap(vector<int> &edge, const Layout &cur_metal, const Layout &temp,
                                int distance, int start, int end)
{
    int length = 0;
    for (int i = start; i < end; i++) {
        if (edge[i] == 0) {
            length++;
            edge[i] = 1;
        }
    }
    
    // no fringe cap between metals that are in same layer or projection overlapping but still cause shielding
    if ((cur_metal.layer == temp.layer) ||
        !(temp.tr_x <= cur_metal.bl_x || temp.bl_x >= cur_metal.tr_x ||
          temp.bl_y >= cur_metal.tr_y || temp.tr_y <= cur_metal.bl_y))
        return length;

    // fringe capacitance
    double fringe_cap = 0.0;
    if (length != 0)
        fringe_cap = CalculateCouplingCapacitance("fringe", cur_metal.layer, temp.layer, distance, length);
    cap[IdToIndex(cur_metal.id - 1, temp.id - 1)] = fringe_cap;

    return length;
}

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
        for (int metal_id : candidate_metals) {
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

        // the nearer to the cur_metal, the sooner the metal should be evaluate
        // consider the projection overlapping metals the same, their orders don't matter
        //     just make sure they are in front of other metals 
        sort(up.begin(), up.end(), [](const int &i, const int &j) {
            return layouts[i].bl_y < layouts[j].bl_y;
        });
        sort(down.begin(), down.end(), [](const int &i, const int &j) {
            return layouts[i].tr_y > layouts[j].tr_y;
        });
        sort(left.begin(), left.end(), [](const int &i, const int &j) {
            return layouts[i].tr_x > layouts[j].tr_x;
        });
        sort(right.begin(), right.end(), [](const int &i, const int &j) {
            return layouts[i].bl_x < layouts[j].bl_x;
        });

        // up
        int width = cur_metal.tr_x - cur_metal.bl_x;
        vector<int> edge(width, 0);
        // remaining fringe edge
        int remaining_length = width;
        for (int metal_id : up) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_y - cur_metal.tr_y;
            // raw intersect length (without shielding)
            int x_start = cur_metal.bl_x > temp.bl_x ? cur_metal.bl_x - cur_metal.bl_x : temp.bl_x - cur_metal.bl_x;
            int x_end = cur_metal.tr_x < temp.tr_x ? cur_metal.tr_x - cur_metal.bl_x : temp.tr_x - cur_metal.bl_x;

            remaining_length -= CalculateShieldingFringeCap(edge, cur_metal, temp, distance, x_start, x_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating Fringe capcitance\n");
            //     printf("[Error] error in calculating Fringe edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // down
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : down) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = cur_metal.bl_y - temp.tr_y;
            // raw intersect length (without shielding)
            int x_start = cur_metal.bl_x > temp.bl_x ? cur_metal.bl_x - cur_metal.bl_x : temp.bl_x - cur_metal.bl_x;
            int x_end = cur_metal.tr_x < temp.tr_x ? cur_metal.tr_x - cur_metal.bl_x : temp.tr_x - cur_metal.bl_x;

            CalculateShieldingFringeCap(edge, cur_metal, temp, distance, x_start, x_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating Fringe capcitance\n");
            //     printf("[Error] error in calculating Fringe edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        edge.clear();

        // left
        width = cur_metal.tr_y - cur_metal.bl_y;
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : left) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = cur_metal.bl_x - temp.tr_x;
            // raw intersect length (without shielding)
            int y_start = cur_metal.bl_y > temp.bl_y ? cur_metal.bl_y - cur_metal.bl_y : temp.bl_y - cur_metal.bl_y;
            int y_end = cur_metal.tr_y < temp.tr_y ? cur_metal.tr_y - cur_metal.bl_y : temp.tr_y - cur_metal.bl_y;

            CalculateShieldingFringeCap(edge, cur_metal, temp, distance, y_start, y_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating Fringe capcitance\n");
            //     printf("[Error] error in calculating Fringe edge (smaller than 0)\n");
            //     exit(1);
                break;
        }
        edge.clear();

        // right
        edge.insert(edge.begin(), width, 0);
        remaining_length = width;
        for (int metal_id : right) {
            const Layout &temp = layouts[metal_id];
            // distance between two metals
            int distance = temp.bl_x - cur_metal.tr_x;
            // raw intersect length (without shielding)
            int y_start = cur_metal.bl_y > temp.bl_y ? cur_metal.bl_y - cur_metal.bl_y : temp.bl_y - cur_metal.bl_y;
            int y_end = cur_metal.tr_y < temp.tr_y ? cur_metal.tr_y - cur_metal.bl_y : temp.tr_y - cur_metal.bl_y;

            CalculateShieldingFringeCap(edge, cur_metal, temp, distance, y_start, y_end);
            // if (remaining_length < 0) {
            //     printf("[Error] error in calculating Fringe capcitance\n");
            //     printf("[Error] error in calculating Fringe edge (smaller than 0)\n");
            //     exit(1);
            // }
            if (remaining_length == 0)
                break;
        }
        vector<int>().swap(edge);
    }
}
