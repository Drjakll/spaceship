

int Compute_Action(Ship_Ctrl_Probability *scp){
    
    int vert = GetRandomValue(1,100);
    int hori = GetRandomValue(1,100);
    int shoot = GetRandomValue(1,100);

    

    int left = (int)(100 * scp->horizontal.move[0].probability);
    int stay_h = left + (int)(100 * scp->horizontal.move[1].probability);
    //int right = stay_h + (int)(100 * scp->horizontal.move[2].probability);

    int up = (int)(100 * scp->vertical.move[0].probability);
    int stay_v = up + (int)(100 * scp->vertical.move[1].probability);
    //int down = stay_v + (int)(100 * scp->vertical.move[2].probability);

    int to_shoot = (int)(100 * scp->shoot.shoot[0].probability);
    //int no_shoot = no_shoot + (int)(100 * scp->shoot.shoot[1].probability);

    int move = 0b0;

    if(hori <= left){
        move |= LEFT;
    } else if(hori > stay_h){
        move |= RIGHT;
    }

    if(vert <= up){
        move |= UP;
    } else if(vert > stay_v){
        move |= DOWN;
    }

    if(shoot <= to_shoot){
        move |= SHOOT;
    }

    return move;
}

bool Compute_Approach(Ship_Ctrl_Approach_Prob *scap){

    int yes_approach = GetRandomValue(1, 100);
    int no_approach = GetRandomValue(1, 100);

    if(yes_approach <= (int)(100 * scap->approach[0].probability)){
        return true;
    } else {
        return false;
    }

}
