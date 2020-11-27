var config_info_origin = {
    "event_thread_pool_size":	2,          //0
    "event_trigger_threads":	2,			//0
    "event_trigger_queue_size":	2000, 		//0
    "event_network_io_threads":	4,		//0
    "dns_upload_interval_ms":	60000,		//* dns上传间隔
    "dns_server_domain":	"www.meanning.com", //* dns域名
    "dns_server_port":	10000, //* dns服务器域名
    "dns_domain_name":	"www.test2.com", //$ 本地服务器域名
    "dns_account_name":	"admin", //$ 本地服务器所属账号
    "dns_pass_word":	"micagent",//0
    "dns_description":	"test_dns",//0
    "balance_upload_interval_ms":	2000, //* 负载均衡服务器上传间隔
    "balance_server_domain":	"www.meanning.com",//* 负载均衡域名
    "balance_server_port":	10001,//* 负载均衡服务器端口
    "balance_max_payload_size":	25,//* 负载均衡服务器最大负载大小
    "balance_server_weight":	0.500000,//* 负载均衡服务器权重 0-1.0
    "rtsp_server_port":	8554,//* rtsp服务端口
    "rtsp_account_name":	"admin",//* rtsp服务器账号名
    "rtsp_account_password":	"micagent",//* rtsp服务器密码
    "rtsp_proxy_port":	8555,//* 转发服务内网端口
    "set_external_ip":	true,//0
    "router_ip":	"",//* 路由器ip
    "rtsp_server_external_port":	8554,//* rtsp服务器外网端口
    "rtsp_proxy_external_port":	8555,//* 转发服务外网端口
    "log_open":	true,//* 是否打开log输出
    "log_path":	"./log"//0 log输出目录
};
var config_info_current = {};
var difference ={};
// 获取服务器配置
function get_config() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd":"get_config"}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            try{
                config_info_origin = data.response.json_config;
                set_config();
            }
            catch (e) {
                console.log(e);
            }
        },
    });
}

// 转json对象为字符串
function transferString(json_obj) {
    return JSON.stringify(json_obj)
}

// 输入框赋值
function set_config() {
    for(let key in config_info_origin){
        try{
            if(key=='log_open') {
                $("#" + key).prop('checked', config_info_origin[key]);
            }
            else {
                $("#" + key).val(config_info_origin[key]);
            }
        }
        catch (e) {
        }
    }
    config_info_current = {};
}


// 配置立即生效
function reload_mode() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd":"reload_mode"}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            try {
                console.log(data.response.info);
            }
            catch (e) {
            }
        },
    });
}

// 保存配置
function save_config() {
    update_config();
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd":"save_config"}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            try {
                console.log(data.response.info);
            }
            catch (e) {

            }
        },
    });
}


// 更新配置
function update_config() {
    get_difference();
    if (JSON.stringify(difference) == "{}") {

    } else {
        $.ajax({
            type: 'post',
            url: '../cgi-bin/web_function.cgi',
            data: transferString({"cmd": "update_config", "config": difference}),
            contentType: 'application/json; charset=utf-8',
            dataType: 'json',
            async: true,
            success: function (data) {
                try{
                    console.log(data.response.info);
                    for (let key in difference)
                    {
                        config_info_origin[key] = difference[key];
                    }
                    config_info_current={};
                }
                catch (e) {

                }
            },
        });
    }
}

// 获取url信息
function get_url_info() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd": "get_url_info"}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            let html="";
            for (let value of data.response.url_info) {
                html+='<li><a>'+value+'</a></li>';
            }
            $(".url_list ul").html(html);
        },
    });
}

// 获取用户更改的配置项
function get_difference() {
    difference ={}
    for (let key in config_info_current)
    {
        if(config_info_origin[key] !== config_info_current[key]) {
            difference[key] = config_info_current[key];
        }
    }
}

//设备网络配置
var net_config = {
    index: 0,
    list: [],
    net_temp: {}
}
function get_net_config() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd": "get_net_config"}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            if ('net_info_list' in data.response) {
                net_config.list = data.response.net_info_list
                if (net_config.list.length < 2) {
                    $('.net_next').attr("disabled", true)
                }
                set_net_config()
            } else {
                console.log('未获取到设备的网络配置信息！')
            }
        },
    });
}

// 输入框赋值
function set_net_config() {
    for(let key  in net_config.list[net_config.index]){
        $("#" + key).val(net_config.list[net_config.index][key]);
    }
    net_config.net_temp = net_config.list[net_config.index]
    net_config.list.length === 0
        ? $(".net_index").text(net_config.index)
        : $(".net_index").text(net_config.index + 1)
    $(".net_count").text(net_config.list.length)
}

//切换上台设备
function primary_config(dom) {
    net_config.index --
    if(net_config.index === 0) {
        $(dom).attr("disabled", true)
    } else  if (net_config.index === net_config.list.length - 2) {
        $(dom).prev().attr("disabled", false)
    }
    set_net_config()
}

//切换下台设备
function next_config(dom) {
    net_config.index ++
    if (net_config.index === net_config.list.length - 1) {
        $(dom).attr("disabled", true)
    } else if (net_config.index === 1) {
        $(dom).next().attr("disabled", false)
    }
    set_net_config()
}

//获取网络配置
function update_net_info() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd": "update_net_info", "net_config": net_config.net_temp}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            try {
                if (data.response.net_set_state) {
                    console.log('网络设置成功！')
                } else {
                    console.log('网络设置失败！')
                }
            }
            catch (e) {
                console.log('请求失败！')
            }
        },
    });
}

//设备重启
function restart() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data: transferString({"cmd":"restart"}),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async:true,
        success:function (data) {
            try {
                if (data.response.net_set_state) {
                    console.log('设备重启成功！')
                } else {
                    console.log('设备重启失败！')
                }
            }
            catch (e) {
                console.log('请求失败！')
            }
        },
    });
}

// 捕捉输入框变化2
function text_change2(obj, state) {
    let key=obj.getAttribute("name");
    let value=obj.value;
    net_config.net_temp[key]=value
}

// 捕捉输入框变化
function text_change(obj, state, type) {
    let value = null
    let key=obj.getAttribute("name");
    if (state) {
        value=obj.value
    } else {
        type === 'int' ? value=Number(obj.value) : value=parseFloat(obj.value)
    }
    if(key=='log_open'){
        value=$(obj).prop('checked');
    }
    config_info_current[key]=value;
}

