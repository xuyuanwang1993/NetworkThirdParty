var config_info_origin = {"event_thread_pool_size":	2,          //0
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
        data: {"cmd":"get_config"},
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
        data: {"cmd":"reload_mode"},
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
        data: {"cmd":"save_config"},
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

    }
    else {
        $.ajax({
            type: 'post',
            url: '../cgi-bin/web_function.cgi',
            data: {"cmd": "update_config", "config": difference},
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
        data: {"cmd":"get_url_info"},
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

// 捕捉输入框变化
function text_change(obj) {
    let key=obj.getAttribute("name");
    let value=obj.value;
    if(key=='log_open'){
        value=$(obj).prop('checked');
    }
    config_info_current[key]=value;
}
