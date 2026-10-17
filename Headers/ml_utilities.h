struct Reg_Data { 
    float* weight;
    float bias;
    float z;
    double exp_e;
    float probability;
};

struct Ship_Ctrl_Shoot_Prob {
    Reg_Data shoot[2];
};

struct Ship_Ctrl_Approach_Prob {
    Reg_Data approach[2];
};

struct Ship_Ctrl_Move_Prob {
    Reg_Data move[3];
    bool avoid_zone;
};

struct Coordinate_Probability {
    Ship_Ctrl_Shoot_Prob shoot_prob[(int)(2 * WINDOW_WIDTH/MAP_FACTOR)][(int)(2 * WINDOW_HEIGHT/MAP_FACTOR)];
    Ship_Ctrl_Move_Prob move_prob_h[(int)(2 * WINDOW_WIDTH/MAP_FACTOR)][(int)(2 * WINDOW_HEIGHT/MAP_FACTOR)];
    Ship_Ctrl_Move_Prob move_prob_v[(int)(2 * WINDOW_WIDTH/MAP_FACTOR)][(int)(2 * WINDOW_HEIGHT/MAP_FACTOR)];
    Ship_Ctrl_Approach_Prob* approach_prob;
};

struct Ship_Ctrl_Probability {
    Ship_Ctrl_Move_Prob horizontal;
    Ship_Ctrl_Move_Prob vertical;
    Ship_Ctrl_Shoot_Prob shoot;
};


void Init_Reg_Data(Reg_Data *rd, float size, int weight_size, float weight_value, float bias){

    rd->weight = calloc(weight_size, sizeof(float));

    for(int i = 0; i < weight_size; i++){
        rd->weight[i] = (float)weight_value/(float)weight_size;
    }

    rd->bias = bias;
    rd->z = 0.0f;
    rd->exp_e = 0.0f;
    rd->probability = 1.0/size;
}

void Init_Ctrl_Move_Prob(Ship_Ctrl_Move_Prob *scmp, int size){

    for(int i = 0; i < size; i++){

        Reg_Data* rd = &scmp->move[i];

        Init_Reg_Data(rd, (float)size, 3, 1, 1);

    }

    scmp->avoid_zone = false;
}

void Init_Shoot_Ctrl_Prob(Ship_Ctrl_Shoot_Prob *scsp){

    for(int i = 0; i < 2; i++){

        Reg_Data* rd = &scsp->shoot[i];

        Init_Reg_Data(rd, 2.0, 2, 1, 1);
    }
}

void Init_Approach_Ctrl_Prob(Ship_Ctrl_Approach_Prob* scap){

    for(int i = 0; i < 2; i++){

        Reg_Data* rd = &scap->approach[i];

        Init_Reg_Data(rd, 2.0, 2, 1, 1);
    }
}

void Init_Ship_Ctrl_Prob(Ship_Ctrl_Probability *scp){

    Ship_Ctrl_Move_Prob *scmp_v = &scp->vertical;
    Ship_Ctrl_Move_Prob *scmp_h = &scp->horizontal;

    Ship_Ctrl_Shoot_Prob *scsp = &scp->shoot;

    Init_Ctrl_Move_Prob(scmp_v, 3);
    Init_Ctrl_Move_Prob(scmp_h, 3);
    Init_Shoot_Ctrl_Prob(scsp);

}

void Init_Coord_Prob(Coordinate_Probability *cp) {

    int x = (2 * WINDOW_WIDTH/MAP_FACTOR);
    int y = (2 * WINDOW_HEIGHT/MAP_FACTOR);

    cp->approach_prob = calloc(distance_size, sizeof(Ship_Ctrl_Approach_Prob));

    for(int i = 0; i < x; i++){

        for(int j = 0; j < y; j++){

            Ship_Ctrl_Shoot_Prob* scsp = &cp->shoot_prob[i][j];
            Ship_Ctrl_Move_Prob* scmp_h = &cp->move_prob_h[i][j];
            Ship_Ctrl_Move_Prob* scmp_v = &cp->move_prob_v[i][j];

            Init_Shoot_Ctrl_Prob(scsp);
            Init_Ctrl_Move_Prob(scmp_h, 3);
            Init_Ctrl_Move_Prob(scmp_v, 3);
        }
    }

    for(int i = 0; i < distance_size; i++){

        Ship_Ctrl_Approach_Prob *scap = &cp->approach_prob[i];

        Init_Approach_Ctrl_Prob(scap);
    }

}

float Clipping(float old_p, float new_p){

    float e = 0.1;

    float ratio = new_p/(old_p == 0.0f ? 0.01 : old_p) ;

    if(ratio > 1.0 + e){

        new_p = old_p * (1.0 + e);

    } else if (ratio < 1.0 - e){

        new_p = old_p * (1.0 - e);

    }

    return new_p;
}

void Calc_Prob(Reg_Data orig_rd[], float input[], int size, int input_size, bool clip){
    
    double total_e = 0;

    
    for(int i = 0; i < size; i++){

        float total_input = 0.0f;

        for(int j = 0; j < input_size; j++){
            total_input += input[j] * orig_rd[i].weight[j];
        }

        orig_rd[i].z = total_input + orig_rd[i].bias;

    }

    float max_z = orig_rd[0].z;

    for(int i = 0; i < size; i++){

        if(orig_rd[i].z > max_z){

            max_z = orig_rd[i].z;

        }
    }
    

    for(int i = 0; i < size; i++){

        orig_rd[i].exp_e = exp(orig_rd[i].z - max_z);


        total_e += orig_rd[i].exp_e;
    }

    float fill = 0.0f;

    for(int i = 0; i < size; i++){

        float probability = orig_rd[i].exp_e/total_e;

        //probability = clip ? Clipping(orig_rd[i].probability, probability) : probability;

        fill += probability;

        orig_rd[i].probability = probability;

    }

    for(int i = 0; i < size; i++){

        orig_rd[i].probability /= fill;

    }
}

void Multi_Logistic_Regression(Reg_Data orig_rd[], float lr, float answer[], float input[], int size, int input_size){

    //printf("iteration %i\n\n", count);

    count++;

    //printf("original\n");

    Calc_Prob(orig_rd, input, size, input_size, false);

    for(int i = 0; i < size; i++){

        for(int j = 0; j < input_size; j++){
            orig_rd[i].weight[j] -= (lr * (orig_rd[i].probability - answer[i]) * input[j]);
        }

        float loss_over_bias = (orig_rd[i].probability - answer[i]);

        float new_bias = orig_rd[i].bias - lr * loss_over_bias;

        orig_rd[i].bias = new_bias;

    }

    Calc_Prob(orig_rd, input, size, input_size, true);


}

Vector2 Calc_XY_Coord_Difference(Vector2 target2, Vector2 target1){

    Vector2 result = {.x = target2.x - target1.x, .y = target2.y - target1.y};

    return result;

}


