#pragma once
#include <vector>
static std::vector<unsigned char> space_device_bytes(const void *pointer, size_t bytes) {
    std::vector<unsigned char> data(bytes);
    assert(cudaMemcpy(data.data(),pointer,bytes,cudaMemcpyDeviceToHost)==cudaSuccess);
    return data;
}
static std::vector<precision_t> space_device_values(Prec tensor) {
    std::vector<precision_t> data(numel(tensor.shape));
    assert(cudaMemcpy(data.data(),tensor.data,data.size()*sizeof(precision_t),cudaMemcpyDeviceToHost)==cudaSuccess);
    return data;
}
static void space_check_near(float actual, double expected) {
    assert(isfinite(actual) && fabs(actual-expected)<.0002*fmax(1.0,fabs(expected)));
}
static void space_check_rollout(Ini *ini, TrainContext *ctx) {
    assert(!USE_BF16);
    space_check_actions=1;
    PuffeRL *p=create_pufferl(ini,ctx);
    assert(!p->hypers.async && !p->hypers.vtrace && p->num_policies==1 && p->vec->buffers==1);
    int H=p->hypers.horizon,B=p->hypers.total_agents;
    int ships=p->vec->envs[0].num_agents,period=p->vec->envs[0].diagnostic_period;
    assert(H==64 && period>0 && p->vec->policy_layout[1]==B);
    assert(numel(p->policies[0].master_weights.shape)==188800);
    for(int w=0;w<p->vec->size;++w) for(int a=0;a<ships;++a) {
        assert(p->vec->envs[w].agents[a].policy==0);
        assert(p->vec->envs[w].agents[a].actions==p->vec->actions+(w*ships+a)*2);
    }
    std::vector<float> expected_rewards(3*H),expected_dones(3*H);
    bool first_alive=true;
    for(int t=1;t<=3*H;++t) {
        float r=(t%67==63 ? 1.f:0.f)-(t%67==64 ? 2.f:0.f);
        int living=ships-(first_alive?0:1);
        if(t%period==0) {r-=1.02f*living;expected_dones[t-1]=1;first_alive=true;}
        else if(t%67==65 && ships>1 && first_alive) {r-=1.02f;first_alive=false;}
        expected_rewards[t-1]=r;
    }
    long checked=0;
    std::vector<precision_t> previous_bootstrap;
    for(int k=0;k<3;++k) {
        rollouts(p);
        auto values=space_device_values(p->rollouts.values);
        auto rewards=space_device_values(p->rollouts.rewards);
        auto dones=space_device_values(p->rollouts.terminals);
        if(!previous_bootstrap.empty()) for(int a=0;a<B;++a)
            space_check_near(to_float(values[a]),to_float(previous_bootstrap[a]));
        auto rng=space_device_bytes(p->rng_states[0],B*sizeof(curandStatePhilox4_32_10_t));
        auto actions=space_device_bytes(p->env.actions.data,B*2*sizeof(float));
        Prec carry=p->policies[0].buffer_states[0];
        auto state=space_device_bytes(carry.data,numel(carry.shape)*sizeof(precision_t));
        std::vector<Env> worlds(p->vec->envs,p->vec->envs+p->vec->size);
        space_prepare_bootstrap(p,p->train_stream);
        assert(cudaStreamSynchronize(p->train_stream)==cudaSuccess);
        assert(rng==space_device_bytes(p->rng_states[0],rng.size()));
        assert(actions==space_device_bytes(p->env.actions.data,actions.size()));
        assert(state==space_device_bytes(carry.data,state.size()));
        assert(!memcmp(worlds.data(),p->vec->envs,worlds.size()*sizeof(Env)));
        previous_bootstrap=space_device_values(p->space_bootstrap_values);
        transpose_102<<<grid_size(H*B),BLOCK_SIZE,0,p->train_stream>>>(p->train_rollouts.values.data,p->rollouts.values.data,H,B,1);
        transpose_102<<<grid_size(H*B),BLOCK_SIZE,0,p->train_stream>>>(p->train_rollouts.rewards.data,p->rollouts.rewards.data,H,B,1);
        transpose_102<<<grid_size(H*B),BLOCK_SIZE,0,p->train_stream>>>(p->train_rollouts.terminals.data,p->rollouts.terminals.data,H,B,1);
        space_advantage<<<grid_size(B),BLOCK_SIZE,0,p->train_stream>>>(
            p->train_rollouts.values.data,p->train_rollouts.rewards.data,p->train_rollouts.terminals.data,
            p->env.rewards.data,p->env.terminals.data,p->space_bootstrap_values.data,
            p->train_rollouts.logprobs.data,p->train_rollouts.values.data,.99f,.95f,B,H);
        assert(cudaStreamSynchronize(p->train_stream)==cudaSuccess);
        auto adv=space_device_values(p->train_rollouts.logprobs);
        auto returns=space_device_values(p->train_rollouts.values);
        for(int a=0;a<B;++a) for(int t=0;t<H;++t) {
            float r=t+1==H?p->vec->rewards[a]:to_float(rewards[(t+1)*B+a]);
            float d=t+1==H?p->vec->terminals[a]:to_float(dones[(t+1)*B+a]);
            space_check_near(r,expected_rewards[k*H+t]);
            assert(d==expected_dones[k*H+t]);
            double expected=0,weight=1;
            for(int j=t;j<H;++j) {
                int index=k*H+j;
                double next=j+1==H?to_float(previous_bootstrap[a]):to_float(values[(j+1)*B+a]);
                expected+=weight*(expected_rewards[index]+.99*(1-expected_dones[index])*next-to_float(values[j*B+a]));
                if(expected_dones[index]) break;
                weight*=.99*.95;
            }
            space_check_near(to_float(adv[a*H+t]),expected);
            space_check_near(to_float(returns[a*H+t]),to_float(values[t*B+a])+expected);
            ++checked;
        }
    }
    auto before=space_device_bytes(p->policies[0].master_weights.data,188800*sizeof(float));
    for(int update=0;update<2;++update) {
        rollouts(p); train_impl(p,NULL);
        std::vector<float> losses(NUM_LOSSES);
        assert(cudaMemcpy(losses.data(),p->losses,losses.size()*sizeof(float),cudaMemcpyDeviceToHost)==cudaSuccess);
        for(float loss:losses) assert(isfinite(loss));
    }
    auto after=space_device_bytes(p->policies[0].master_weights.data,188800*sizeof(float));
    assert(before!=after);
    puf_save_weights(p,"build/space-diagnostic.bin");
    pufferl_load_policy(p,0,"build/space-diagnostic.bin");
    assert(after==space_device_bytes(p->policies[0].master_weights.data,after.size()));
    printf("SPACE_CHECK_JSON {\"ships\":%d,\"period\":%d,\"passed\":true,\"bootstrap_preserved\":true,"
        "\"action_logprob_checked\":true,\"optimizer_changed\":true,\"checkpoint_roundtrip\":true,\"checked_targets\":%ld}\n",ships,period,checked);
    close_pufferl(p);
}
