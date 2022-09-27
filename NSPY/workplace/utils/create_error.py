from random import randint, sample

def cal_gp(src,k):

    mu = k*k*5//4
    pod = (src-mu)//(k*k//4)
    lala = k*k//4 + k*pod + k//2 + (src-mu-pod*k*k//4)//(k//2)
    return lala

def create_error(hosts,typ,num,k):
    ans=dict()
    ans2=dict()
    mark=dict()
    error_host = set()
    num_core = k*k//4
    err_hosts = sample(hosts, num)
    cores = range(num_core)
    tmp_ = 0
    if typ==0 or typ==2 or typ==3:
        for host in err_hosts:
                gr=cal_gp(host,k)
                # core_node = randint(0,num_core-1)
                # core_nodes = sample(cores,num_core//4)
                core_nodes = sample(cores,num_core//16)
                gr_host = (gr<<12) + host

                ans[gr_host] = core_nodes
                # ans[gr_host] = [core_node]
                # mark[core_node] = 1
                error_host.add(gr_host)

                # break

    elif typ==1:
        for host in err_hosts:
            gr=cal_gp(host,k)
            # core_nodes = sample(cores,num_core//4)
            core_nodes = sample(cores,num_core//16)
                # core_node = randint(0,num_core-1)
            gr_host = (gr<<12) + host
                # if core_node in mark.keys():
                    # continue
            ans[gr_host]=[]
            error_host.add(gr_host)
            for core_node in core_nodes:
                if core_node in ans2.keys():
                    ans[gr_host].extend([core_node,ans2[core_node]])
                else:
                    for pod in range(k):
                        aggr_node = num_core + (core_node // (k // 2)) + (k * pod)
                        if aggr_node in ans2.keys():
                            continue
                        # ans[gr_host] = [core_node,aggr_node]
                        ans[gr_host].extend([core_node,aggr_node])

                        # mark[aggr_node] = 1
                        ans2[core_node] = aggr_node
                        ans2[aggr_node] = core_node
                        # error_host.add(gr_host)
                        # error_host.add(aggr_node)

                        break

                # break
                
    return error_host, ans, ans2