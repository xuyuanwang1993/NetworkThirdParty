//获取保存了多少个预案
function getSceneInfo() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SceneInfo\r\n',
        async:true,
        success:function (data) {
                console.log(data);
                if($.trim(data)==""){
                    //alert("获取多少个预案信息失败！"+data);
                }
        },
    });
}

//数据集同步，获取IPC信息
function getDataSyncOne(){
    var ipc_id="",user="",ipaddr="",ipc_width="",ipc_height="",shi_width="",shi_height="",ipArr =[],idArr ="",name="",trid="",device_type=0;
    //获取IPC信息
    $.ajax({
        type:"post",
        url: '../cgi-bin/web_function.cgi',
        data:'cmd : DataSync\r\ntype : 1\r\n',
        async:false,
        success:function (data) {
                if($.trim(data)=="error"||$.trim(data)==""){
                    alert("获取IPC信息失败！");
                }else{
                    var array=data.split("\r\n");
                    var array1=data.split("\r\n");
                    var html="";
                    var ipc_Data="";
                    var ipc_swiper="";
                    var con=1;
                    var suc=1;
                    var suu = 1;
                    console.log(data);
                    for (var i=0; i<array.length;i++){
                        if ($.trim(array[i]).indexOf("ipaddr") >= 0) {//地址
                            ipaddr = ($.trim(array[i].split(' : ')[1]));
                        } else if ($.trim(array[i]).indexOf("name") >= 0) {
                            name = $.trim(array[i].split(' : ')[1]);
                        } else if ($.trim(array[i]).indexOf("user") >= 0) {
                            user = $.trim(array[i].split(' : ')[1]);
                        } else if ($.trim(array[i]).indexOf("id ") >= 0) {
                            ipc_id = $.trim(array[i].split(': ')[1]);
                        } else if ($.trim(array[i]).indexOf("device_type") >= 0) {
                            device_type = $.trim(array[i].split(': ')[1]);
                        } else if ($.trim(array[i]).indexOf("width") >= 0) {
                            var width = $.trim(array[i].split(' : ')[1]);
                            ipc_width = $.trim(width.split(':')[0]);
                            shi_width = $.trim(width.split(':')[1]);
                        } else if ($.trim(array[i]).indexOf("height") >= 0) {
                            var height = $.trim(array[i].split(' : ')[1]);
                            ipc_height = $.trim(height.split(':')[0]);
                            shi_height = $.trim(height.split(':')[1]);
                            if(parseInt(device_type)==1){
                                html+='<div class="tree-xin swiper-slide" name="'+device_type+'">'+ipaddr+ '' +
                                     '<input type="hidden" id="'+ipc_id+'" name="'+ipc_width+'" class="'+ipc_height+'" value="">'+
                                     '<sapn class="wen-ip">多应用'+name+'</sapn>'+
                                     '</div>';
                             }else if (parseInt(device_type)==0){
                                 ipc_Data+= '<div class="nav-imgs seni animation" name="'+device_type+'">'+ipaddr+ '' +
                                     '<input type="hidden" id="'+ipc_id+'" name="'+ipc_width+'" class="'+ipc_height+'" value="">'+
                                     '<canvas class="wen-ip">'+name+'</canvas>'+
                                     '</div>';
                             }
                             trid += "<tr>" +
                                      "<td class='signal_choose'><input type='checkbox' name='checktoo' /></td>" +
                                      "<td class='signal_number'>"+(suu++)+"</td>" +
                                      "<td class='signal_id'>"+ipc_id+"</td>" +
                                      "<td class='signal_ipc'>"+ipaddr+"</td>" +
                                      "<td class='signal_user_name'>"+user+"</td>" +
                                      "<td class='signal_name'>"+name+"</td>" +
                                      "<td></td>" +//<input class='signal_alter' type='button' value='修改'/>
                                      "</tr>";
                        }
                        $('#tree .swiper-wrapper-wen').html(html);
                        $('#tree #tree-pc #enen').html(html);
                        $("#div-swp #en").html(ipc_Data);
                        $("#list-too").html(trid);
                    }
                    var shuzu=idArr.split("\r\n");

                    //视频监控轮播操作
                    var marginWidth=$(".animation").outerWidth(true);
                    var con=0-parseInt(marginWidth);

                    /*按钮 下一个*/
                    $("#butleft").off('click').on("click",function(){
                        $("#en").animate({"margin-left":con},function(){
                            $("#en").append($(".animation").eq(0));
                            $("#en").css("margin-left","0px");
                        });
                    });
                    /*按钮上一个*/
                    $("#butright").off('click').on("click",function(){
                        $("#en").prepend($("#en div:last"));
                        $("#en").css("margin-left",con);
                        $("#en").animate({"margin-left":"0px"},function(){

                        });
                    });

                    //修改
                    $("#list-too").delegate(".signal_alter", "click", function () {
                        $(".alt_erbox").show();
                        //找到当前按钮所在td的之前的所有td，因为数据就在这些td中
                        var tds = $(this).parent().prevAll();
                        //设置文本框的值
                        $('#hidId').val(tds.eq(3).text());//用于存放修改行的id。并用于判别是添加操作还是修改。
                        $('#input_ka').val(tds.eq(2).text());
                        $('#input_miao').val(tds.eq(1).text());
                        $('#input_die').val(tds.eq(0).text());
                        $('.signal_alter').css("background", "#c9cc19");
                        setTimeout(function () {
                            $(".signal_alter").css("background", "transparent");
                        }, 200);
                    });
                }
            },
    });
}

var width="",height="",mode,inp_row="",inp_col="";
//设置幕墙，获取一个频开窗个数
function getGetWinMode(){
    $.ajax({
        type:"post",
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : GetWinMode\r\n',
        async:false,
        success:function (data) {
                if ($.trim(data)=="error"||$.trim(data)==""){
                    alert("获取一个频开窗个数信息失败！");
                }else {
                    var array=data.split("\r\n");
                    for (var i=0;i<array.length;i++){
                        if ($.trim(array[i]).indexOf("mode") >= 0) {//个数
                            mode = $.trim(array[i].split(' : ')[1]);
                        }
                    }
                    if (mode==4){
                       mode=mode/2;
                    } else if (mode==9){
                       mode=mode/3;
                    } else if (mode==16){
                       mode=mode/4;
                    } else if (mode==25){
                       mode=mode/5;
                    }
                }
        },
    });
    $.ajax({
        type:"post",
        url: '../cgi-bin/web_function.cgi',
        data:'cmd : WallSet\r\n',
        async: false,
        success:function (data) {
                if($.trim(data)=="error"||$.trim(data)==" "){
                    alert("设置幕墙失败！");
                }else{
                    var array = data.split("\r\n");
                    var tb_colTmp = "";
                    var tb_row = "";
                    var tb_rowTmp = "";
                    var cont=0;
                    td_cont=cont;
                    var sum=0;
                    var tb_cl="";
                    for (var i = 0; i < array.length; i++) {
                        if ($.trim(array[i]).indexOf("row") >= 0) {//行
                            inp_row = $.trim(array[i].split(' : ')[1]);
                        }else if($.trim(array[i]).indexOf("col ") >= 0){
                            inp_col=$.trim(array[i].split(': ')[1]);
                            break;
                        }
                    }
                    for (var i = 0; i < inp_row; i++) {
                        var tableData = "<tr style='border: 1px #3fe6f7 solid'>";
                        for (var j = 0; j < inp_col; j++) {
                            tb_colTmp = '<td id="com' + (cont++) + '" class="tbs" style="border: 2px #3fe6f7 solid;position: relative;"><table id="tb-table" border="1" cellpadding="0" cellspacing="0">';
                                for (var k = 0; k < mode; k++) {
                                    var ta_tb='<tr>';
                                for (var a = 0; a < mode; a++) {
                                    tb_cl = '<td id="row_col_' + (sum++) + '"style="position: relative;"></td>';
                                    ta_tb+=tb_cl;
                                }
                                tb_colTmp+=ta_tb+'</tr>';
                                }
                                tableData += tb_colTmp+ '</table></td>';
                        }
                        tb_rowTmp = tableData + "</tr>";
                        tb_row += tb_rowTmp;
                    }
                    $("#tb").html(tb_row);
                    $("#input_row option[value='"+inp_row+"']").attr('selected','selected');
                    $("#input_col option[value='"+inp_col+"']").attr('selected','selected');

                    var leng=$("#ping-zu option").length;
                    var tb_com=$("#tb").find("td:first").attr('id');
                        tb_com=tb_com.replace(/[^0-9]/ig,"");
                    var end_Id=parseInt(tb_com)+((inp_row*inp_col)-1);
                    var pen_div="<div id='ping"+leng+"' name='"+inp_row+"' value='"+inp_col+"' class='col-md-4 ping pm name_lcd'>" +
                           "<span class='wen-ip' name='"+tb_com+"' value='"+end_Id+"'>默认幕墙组</span>" +
                           "</div>";
                    $(pen_div).appendTo($("#div-pin")).css({
                            width:'22%',
                            height: '50%',
                            border: 'none',
                            marginBottom: '3%',
                            marginRight: '3%',
                            background:"#13518e",
                            position: 'relative',
                            textAlign: 'center',
                            wordWrap: 'break-word',
                            wordBreak: 'normal',
                            pointerEvents: 'auto',
                            zIndex:13,
                        });

                        var wi=($('#div-pin .ping').width());
                        var heig=($('#div-pin .ping').height());
                        $('#div-pin .ping').draggable({
                            helper:"clone",
                            cursor: "move",
                            scroll:false,
                            start: function( event, ui ) {
                                $(ui.helper).css({
                                    width:wi+'px',
                                    height:heig+'px',
                                    zIndex:13,
                                });
                            },
                            stop:function (event,ui) {
                                var pin_name = $(this).attr("name");
                                var pin_vlaue = $(this).attr("value");
                                var tb_colTmp = "";
                                var tb_rowTmp = "";
                                var tb_row = "";
                                var cont=$(this).find(' span').attr('name');
                                cont=parseInt(cont);
                                var sum=0;
                                var tb_cl="";
                                for (var i = 0; i < pin_name; i++) {
                                    var tableData = "<tr style='border: 1px #3fe6f7 solid'>";
                                    for (var j = 0; j < pin_vlaue; j++) {
                                        tb_colTmp = '<td id="com' + (cont++) + '" class="tbs'+i+'" data-name="cv'+j+'" style="border: 2px #3fe6f7 solid;position: relative;"><table id="tb-table" border="1" cellpadding="0" cellspacing="0">';
                                        for (var k = 0; k < 3; k++) {
                                            var ta_tb='<tr>';
                                        for (var a = 0; a < 3; a++) {
                                            tb_cl = '<td id="row_col_' + (sum++) + '" class="xin_tb" style="position: relative;"></td>';
                                            ta_tb+=tb_cl;
                                        }
                                        tb_colTmp+=ta_tb+'</tr>';
                                    }
                                    tableData += tb_colTmp+ '</table></td>';
                                    }
                                    tb_rowTmp = tableData + "</tr>";
                                    tb_row += tb_rowTmp;
                                }
                                $("#tb").html(tb_row);
                                //传输幕墙指令
                                dev_row =pin_name;
                                dev_col =pin_vlaue;
                                setJoinSet();
                            }
                        });
                        //移动端长按删除
                        var s_longTapBox = $(".ping");
                        for (var i=0;i<s_longTapBox.length;i++) {
                            (function(s_longTapBox) {
                                Transform(s_longTapBox);
                                new AlloyFinger(s_longTapBox, {
                                    longTap:function(evt){
                                        $("#pin_name").fadeIn(500);
                                        $("#pin_name").fadeOut(500);
                                        $(s_longTapBox).remove();
                                        evt.preventDefault();
                                    }
                                });
                            })(s_longTapBox[i]);
                        }
                   }
                },
                async:true,
        });
}
/**
 * 获取所有多屏组信息
 */
function getGetScreenGroupInfo() {
    $.ajax({
            type:"post",
            url: '../cgi-bin/web_function.cgi',
            data:'cmd : GetScreenGroupInfo\r\n',
            async:true,
            success:function (data) {
                if($.trim(data)=="error"||$.trim(data)==""){
                    alert("读取IP地址失败");
                }else{
                    var array=data.split("\r\n");
                    var html="";
                    var groupId,groupName,endId,beginId,row,col;
                    for (var i=0; i<array.length;i++){
                        if($.trim(array[i]).indexOf("GroupId")>=0){
                            groupId=($.trim(array[i].split(' : ')[1]));
                        }else if ($.trim(array[i]).indexOf("GroupName")>=0){
                            groupName=$.trim(array[i].split(' : ')[1]);
                        }else if ($.trim(array[i]).indexOf("BeginId")>=0) {
                            beginId=$.trim(array[i].split(' : ')[1]);
                        }else if ($.trim(array[i]).indexOf("EndId")>=0) {
                            endId=$.trim(array[i].split(' : ')[1]);
                        }else if ($.trim(array[i]).indexOf("row")>=0) {
                            row=$.trim(array[i].split(' : ')[1]);
                        }else if ($.trim(array[i]).indexOf("col")>=0) {
                            col=$.trim(array[i].split(': ')[1]);
                            html="<div id='ping"+groupId+"' name='"+row+"' value='"+col+"' class='col-md-4 ping pm name_lcd'>"+groupName+ "" +
                                "<span class='wen-ip' name='"+beginId+"' value='"+endId+"'>LCD</span>" +
                                "</div>";
                            $(html).appendTo($("#div-pin")).css({
                                width:'22%',
                                height: '50%',
                                border: 'none',
                                marginBottom: '3%',
                                marginRight: '3%',
                                background:"#13518e",
                                position: 'relative',
                                textAlign: 'center',
                                wordWrap: 'break-word',
                                wordBreak: 'normal',
                                pointerEvents: 'auto',
                                zIndex:13,
                            });
                        }

                    }
                }
           },
    });
}
/**
 * 搜索的IPC
 */
function getSearchIpc() {
    $.ajax({
            type:"post",
            url: '../cgi-bin/web_function.cgi',
            data:'cmd : SearchIpc\r\n',
            async:false,
            success:function (data) {
                if($.trim(data)==""){
                    alert("IP地址为空");
                }else if($.trim(data)=="error"){
                    alert("读取IP地址失败");
                }else{
                    var array=data.split("\r\n");
                    var ipc_id="";
                    var s_num = 1;
                    var tr="";
                    console.log(data);
                    for (var i=0; i<array.length;i++){
                        if($.trim(array[i]).indexOf("ipc_")>=0){//地址
                            ipc_id=($.trim(array[i].split(' : ')[1]));
                            var str1 = array[i].substring(array[i].indexOf('ipc_')+4,array[i].lastIndexOf(' : '));
                            tr += "<tr id=" + (id++) + ">"+
                                  "<td class='id'><input type='checkbox' name='check' /></td>"+
                                  "<td class='ru_id'>" + (s_num++) + "</td>"+
                                  "<td class='type'>" + str1 + "</td>"+
                                  "<td class='miao_shu'>"+ipc_id+"</td>"+
                                  "<td class='die_jia'></td>"+
                                  "<td></td>"+
                                  "</tr>";
                        }
                        //将获取的tr 追加到 table中
                        $('#list').html(tr);

                    }
                }
           },
    });
}
