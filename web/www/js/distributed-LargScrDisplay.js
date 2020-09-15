/**
 * 输入信号源轮播
 * 选中可修改设置音量控件
 */
var myswiper = new Swiper('.swiper-container-wen',{
    slidesPerView: 'auto',
    freeMode: true,
    direction: 'vertical',
    setWrapperSize: true,
    noSwiping : true,
    scrollbar: {
        el: '.swiper-scrollbar',
    },
    on:{
        sliderMove: function(event){
            event.preventDefault();
        },
    },
    observer:true,
    observeParents:true,
});
$(document).ready(function() {
    $("#d-g-slider-vertical").slider({
        orientation: "vertical",
        value:30,
        min: -1,
        max: 101,
        step: 1,
        range: 'min',
        start: function(event, ui) {
            $("#d-g-sliderValue").show();
        },
        stop: function (event,ui) {
            $("#d-g-sliderValue").fadeOut(300);
        }
    });
    var d_thumb = $($('#d-g-slider-vertical').children('.ui-slider-handle'));
    var d_dsf=$($('#d-g-slider-vertical'));
    dsetLabelPosition();
    $('#d-g-slider-vertical').bind('slide', function () {
        $('#d-g-sliderValue').val($('#d-g-slider-vertical').slider('value'));
        dsetLabelPosition();
    });
    function dsetLabelPosition() {
        var label = $('#d-g-sliderValue');
        label.css('top', (d_thumb.offset().top-d_dsf.offset().top));
        label.css('left', '90%');
    }
    $("#z-k-slider-vertical").slider({
        orientation: "vertical",
        value:30,
        min: -1,
        max: 101,
        step: 1,
        range: 'min',
        start: function(event, ui) {
            $("#z-k-sliderValue").show();
        },
        stop: function (event,ui) {
            $("#z-k-sliderValue").fadeOut(300);
        }
    });
    var z_thumb = $($('#z-k-slider-vertical').children('.ui-slider-handle'));
    var z_dsf=$($('#z-k-slider-vertical'));
    zsetLabelPosition();
    $('#z-k-slider-vertical').bind('slide', function () {
        $('#z-k-sliderValue').val($('#z-k-slider-vertical').slider('value'));
        zsetLabelPosition();
    });
    function zsetLabelPosition() {
        var label = $('#z-k-sliderValue');
        label.css('top', (z_thumb.offset().top-z_dsf.offset().top));
        label.css('left', '90%');
    }
    $("#slider-vertical").slider({
        orientation: "vertical",
        value:30,
        min: -1,
        max: 101,
        step: 1,
        range: 'min',
        start: function(event, ui) {
            $("#sliderValue").show();
        },
        stop: function (event,ui) {
            $("#sliderValue").fadeOut(300);
        }
    });
    var thumb = $($('#slider-vertical').children('.ui-slider-handle'));
    var dsf=$($('#slider-vertical'));
    setLabelPosition();
    $('#slider-vertical').bind('slide', function () {
        $('#sliderValue').val($('#slider-vertical').slider('value'));
        setLabelPosition();
    });
    function setLabelPosition() {
        var label = $('#sliderValue');
        label.css('top', (thumb.offset().top-dsf.offset().top));
        label.css('left', '90%');
    }
});

/**
 * 登陆
 * 设置分配子账号
 * 修改密码
 */
$(document).ready(function () {
    $("#li-6").click(function () {
        $("#admin").toggle("clip");
        $("#admin-pin").show();
        var li_name=$("#li-8").text().trim();
        console.log(li_name);
        if (li_name!="admin"){
            $("#inpwd").css({"pointer-events":"none","cursor":"default","opacity":"0.6"});
            $("#ad-xiu").css("display","block");
            $("#ad-pwd").css("display","none");
            $("#reset").css({"pointer-events":"none","cursor":"default","opacity":"0.6"});
        }
    });
    $("#ad-clo").click(function () {
        $("#admin").hide();
        $("#admin-pin").hide();
    });
    $("#inpwd").click(function () {
        $("#ad-pwd").show();
        $("#ad-xiu").hide();
        $("#ad-reset").hide();
    });
    $("#uppwd").click(function () {
        $("#ad-pwd").hide();
        $("#ad-reset").hide();
        $("#ad-xiu").show();
    });
    $("#reset").click(function () {
        $("#ad-pwd").hide();
        $("#ad-xiu").hide();
        $("#ad-reset").show();
    });
    $("#ad-subm").click(function () {
        var name=document.getElementById("ad-user-name");
        var pwd=document.getElementById("ad-user-pwd");
        var xin_pwd=document.getElementById("ad-xiu-user-pwd");
        var xin_xinpwd=document.getElementById("ad-xiu-user-xinpwd");

        if(name.value==""||name.value==null){
            console.log("请输入用户名："+name.value);
            return false;
        }else if($("#ad-pwd").css("display")=='block'){
            if(pwd.value==""||pwd.value==null){
                console.log("请输入密码："+pwd.value);
                return false;
            }else {
                $("#admin").hide();
                $("#admin-pin").hide();
                return true;
            }
        }else if ($("#ad-xiu").css("display")=='block') {
            if(xin_pwd.value==""||xin_pwd.value==null){
                console.log("请输入旧密码："+xin_pwd.value);
                return false;
            }else if(xin_xinpwd.value==""||xin_xinpwd.value==null){
                console.log("请输入新密码："+xin_xinpwd.value);
                return false;
            }else {
                $("#admin").hide();
                $("#admin-pin").hide();
                return true;
            }
        }else if ($("#ad-reset").css("display")=='block') {
            if(name.value!="admin"){
                console.log("查无此用户："+name.value);
                return false;
            }else {
                $("#admin").hide();
                $("#admin-pin").hide();
                return true;
            }
        }
    });
});

/**
 * 预案功能
 * 中控功能
 * 回显设置操作
 * AI语音操作
 * @type {string}
 */
$(function(){
    $('body').height($('body')[0].clientHeight);
});
var id="",scene_id="",scene_name="",del_id="",loop_scene="",time="",play_count=0,c="",r="",k="",b="";
$(document).ready(function () {
    $("#clo1").click(function () {
        $(".cfposk,#cfpo-ent1").hide({ effect: "drop", direction: "right" });
        $(this).css("background","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("background","#00bcd4");
        },200);
    });
    $("#ent2").click(function () {
        $(".cfposk,#cfpo-ent2").show("drop");
        $("#clo2").css("background","transparent");
        $("#save").css("display","onoe");
    });
    $("#clo2").click(function () {
        $(".cfposk,#cfpo-ent2").hide({ effect: "drop", direction: "right" });
        $(this).css("background","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("background","transparent");
        },200);
    });
    $("#ent3").click(function () {
        $(".cfposk,#cfpo-ent3").show("drop");
        $("#save").css("display","onoe");
    });
    $("#clo3").click(function () {
        $(".cfposk,#cfpo-ent3").hide({ effect: "drop", direction: "right" });
        $(this).css("background","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("background","transparent");
        },200);
    });
    var ent4 =document.getElementById("ent4");
    Transform(ent4);
    new AlloyFinger(ent4, {
        longTap:function(evt){
            $(".cfposk,#cfpo-ent4").show("drop");
            $(ent4).css({"z-index":"12","background":"#c9cc19"});
            evt.preventDefault();
        },
        touchEnd:function (evt) {
            $(".cfposk,#cfpo-ent4").hide("drop");
            $(ent4).css({"background":"transparent","z-index":"10"});
            evt.preventDefault();
        }
    });
    $("#ent5").click(function () {
        $(".cfposk,#cfpo-ent5").show("drop");
    });
    $("#subm5").click(function () {
        var hui_ip=document.getElementById("hui_ip");
        var xian_ip=/(\d+)\.(\d+)\.(\d+)\.(\d+)/g;
        if(hui_ip.value==0||hui_ip.value==""){
            alert("请输入回显IP地址");
        }else if (!xian_ip.test(hui_ip.value)){
            alert("IP地址输入格式错误");
        }else{
            alert("连接成功");
            $(".cfposk,#cfpo-ent5").hide("drop");
        }
        var _this = this;
        $(this).css("background","#c9cc19");
        setTimeout(function(){
            $(_this).css("background","#00bcd4");
        },200);
    });
    $("#clo5").click(function () {
        $(".cfposk,#cfpo-ent5").hide({ effect: "drop", direction: "right" });
        $(this).css("background","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("background","#00bcd4");
        },200);
    });
    $("#ent7").click(function () {
        $(".cfposk,#cfpo-ent7").show("drop");
        $("#save").css("display","onoe");
    });
    $("#clo7").click(function () {
        $(".cfposk,#cfpo-ent7").hide({ effect: "drop", direction: "right" });
        $(this).css("background","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("background","transparent");
        },200);
    });
    $("#yuan-ul").delegate(".sen","click",function(){
        if($(this).hasClass('test_0')){
            id=$(this).parent().parent().attr('id');
            $(this).removeClass('test_0').addClass('active_1');
            if($('.active_1').length==1){
                if(play_count>0){
                    setSceneLoopStop();
                }
                $(".icon-block").fadeIn("show");
                $("#div-pause-play,#pause-span").fadeOut("hide");
                $(".pause-play").removeClass('pause').addClass('play');
                    scene_id=id;
                    setSceneCall();
            }
            if($('.active_1').length==2){
                $("#div-pause-play,#pause-span").fadeIn("show");
                $(".icon-block").fadeOut("hide");
                $(".pause-play").removeClass('pause').addClass('play');
                $("#pause-span").html("场景轮巡");
            }
        }else if($(this).hasClass('active_1')){
            $(this).removeClass('active_1').addClass('test_0');
        }
    });
    $("#yuan-ul").delegate(".diaoyong","click",function () {
        var scrn=$(".diaoyong").parent().attr('id');
        $(this).css("color","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("color","#ffffff");
        },200);
        scene_id=scrn;
        setSceneCall();
    });
    $("#div-p-p").bind('click',function(){
        if ($('.active_1').length>=2) {
            if($(".pause-play").hasClass('play')){
                $(".pause-play").removeClass('play').addClass('pause');
                $("#div-pause-play,#pause-span").fadeIn("show");
                $(".icon-block").fadeOut("hide");
                $("#pause-span").html("关闭轮巡");
                var cos=$(".pause").css("border-color","transparent transparent transparent #da910b");
                var _this = cos;
                setTimeout(function(){
                    $(_this).css("border-color","transparent transparent transparent #ffffff");
                },200);
                play_count++;
                var yuan_time=document.getElementById("yuan-time");
                time=yuan_time.value;
                console.log(time);
                var arr=$("#yuan-ul ul >li input[class][class!='']").map(function (i,n) {
                    return $(n).attr("class");
                });
                var reg = /\d+/g;
                var sun=arr.get().join();
                console.log(sun);
                var art=sun.match(reg);
                var arr=art.join('');
                console.log(arr);
                for (var i= arr.length-1;i>=0;i--){
                    loop_scene+=arr[i];
                }
                console.log(loop_scene);
                setSceneLoop();
            }else if($(".pause-play").hasClass('pause')){
                $(".pause-play").removeClass('pause').addClass('play');
                $("#pause-span").html("场景轮巡");
                var cos=$(".play").css("border-color","transparent transparent transparent #da910b");
                var _this = cos;
                setTimeout(function(){
                    $(_this).css("border-color","transparent transparent transparent #ffffff");
                },200);
                setSceneLoopStop();
            }

        }else if($(".sen").hasClass('test_0')){}
        if($('.active_1').length==1){
            if(play_count>0){
                 setSceneLoopStop();
            }
            $(".icon-block").fadeIn("show");
            $("#div-pause-play,#pause-span").fadeOut("hide");
            $(".pause-play").removeClass('pause').addClass('play');
            scene_id=id;
            setSceneCall();
        }
    });
    $("#fu4").click(function () {
        setPipWidgetAllClose();
        $("#mqxians .tree-xin,#mqxians .nav-imgs").remove();
        $(this).css({"background":"#3fe6f7","color":"#fff"});
        var _this = this;
        setTimeout(function(){
            $(_this).css({"background":"transparent","color":"#00bcd4"});
        },200);
    });
    $("#fu1").click(function () {
        var sa_count=$("#yuan-ul ul li").length;
        document.getElementById("save-input").value="预案"+(sa_count+1);
        $("#save,#save-dis").show("clip");
        $(this).css({"background":"#3fe6f7","color":"#fff"});
        var _this = this;
        setTimeout(function(){
            $(_this).css({"background":"transparent","color":"#00bcd4"});
        },200);
    });
    $("#fu3").click(function () {
        $(this).css({"background":"#3fe6f7","color":"#fff"});
        var _this = this;
        setTimeout(function(){
            $(_this).css({"background":"transparent","color":"#00bcd4"});
        },200);
        $(".cfposk,#fu3-zuoxi,#zuoxi,#zuo-clo").show("drop");
    });
    $("#zuo-clo").click(function () {
        $(".cfposk,#fu3-zuoxi,#zuoxi,#zuo-clo").hide("drop");
    });
    $("#save-button").click(function () {
        var co_name="";
        var sa_name=document.getElementById("save-input").value;
        $(".co_name").each(function () {
            co_name=$(this).html();
            if (co_name==sa_name){
                $("#save_name").html("预案名称重复");
                $("#save_name").fadeIn(1000);
                $("#save_name").fadeOut(1000);
                return false;
            }
        });
        if(co_name!=sa_name){
            c++;r++;k++;b++;
            scene_name=document.getElementById("save-input").value;
            setSceneRecord();
            $("#save,#save-dis").hide("clip");
        }
        $(this).css("color","#c9cc19");
         var _this = this;
        setTimeout(function(){
            $(_this).css("color","#fff");
        },200);
    });
    $("#save-cancel").click(function () {
        $("#save,#save-dis").hide("clip");
        $(this).css("color","#c9cc19");
         var _this = this;
        setTimeout(function(){
            $(_this).css("color","#fff");
        },200);
    });
    $("#yuan-ul").delegate(".reDel","click",function () {
        del_id=$(this).parent().attr('id');
        setSceneDel();
        $(this).parent().remove();
        $(this).css("background","#c9cc19");
        var _this = this;
        setTimeout(function(){
            $(_this).css("background","transparent");
        },200);
    });
    $("#div-s-sho").click(function () {
        $("#div-sho").css("background","#c9cc19");
            $("#xin-dis").css("display","block");
            $("#xin-hao").show({ effect: "slide", direction: "right" });
        setTimeout(function(){
            $('#div-sho').css("background","transparent");
        },200);
    });
    $("#xin-close").click(function () {
        $("#xin-dis").css("display","none");
        $("#xin-hao").hide({ effect: "slide", direction: "left " });
    });
    $("#div-s-lun").click(function () {
        $("#div-lun").css("background","#c9cc19");
        $("#xin-dis").css("display","block");
        $("#pin-hao").show({effect:"slide",direction:"right"});
        setTimeout(function(){
            $("#div-lun").css("background","transparent");
        },200);
    });
    $("#pin-close").click(function () {
        $("#xin-dis").css("display","none");
        $("#pin-hao").hide({ effect: "slide", direction: "left " });
    });
    var d_this="",d_name="";
    $("#tree .tree-xin .wen-ip,#div-swp .nav-imgs .wen-ip").click(function () {
        var tree_name=$(this).text().trim();
        d_this=this;
        d_name=$(this).parent().find(' input').attr("id");
        document.getElementById("up-input").value=tree_name;
        $("#up,#up-dis").show("clip");

    });
    $("#up-button").off('click').on("click",function () {
        var tree_xin="";
        var sd_name=document.getElementById("up-input").value.trim();
        $(".wen-ip").each(function () {
            tree_xin=$(this).text().trim();
            if (tree_xin==sd_name){
                $("#up_name").html("IPC名称重复");
                $("#up_name").fadeIn(1000);
                $("#up_name").fadeOut(1000);
                return false;
            }
        });
        if (sd_name==""||sd_name==null){
            $("#up_name").html("IPC名称不能为空");
            $("#up_name").fadeIn(1000);
            $("#up_name").fadeOut(1000);
            return false;
        }
        if(tree_xin!=sd_name){
            var tr_name=document.getElementById("up-input").value.trim();
            $(d_this).text(tr_name);
            var wen_ip="";
            $(".wen-ip").each(function () {
                wen_ip=$(this).parent().find(' input').attr("id");
                if (wen_ip==d_name){
                    $(this).text(tr_name);
                }
            });
            $("#up,#up-dis").hide("clip");
        }
        $(this).css("color","#c9cc19");
         var _this = this;
        setTimeout(function(){
            $(_this).css("color","#fff");
        },200);
    });
    $("#up-cancel").click(function () {
        $("#up,#up-dis").hide("clip");
        $(this).css("color","#c9cc19");
         var _this = this;
        setTimeout(function(){
            $(_this).css("color","#fff");
        },200);
    });
});

/**
 * 创建幕墙/多屏组幕墙功能
 * @type {number}
 */
var dev_row =0;
var dev_col =0;
var td_cont=0,groupId=0,groupName="",beginId=0,endId=0,row=0,col=0,groupdelete;
$("#ent1").click(function () {
    if($("#copor1").hasClass('copor1-mei')) {
        $("#copor1").removeClass('copor1-mei').addClass('copor1-yin');
    }else{
        $(".p-name,#div-p-name").hide();
        $("#pingMu,#ping-dis").hide();
        $(".cfposk,.bod-name,#cfpo-ent1").show("drop");

        $("#subm1").off('click').on("click",function () {
            var input_row = document.getElementById("input_row");
            var input_col = document.getElementById("input_col");
            var tb_colTmp = "";
            var tb_rowTmp = "";
            var tb_row = "";
            var cont=0;
            var sum=0;
            var tb_cl="";

            var cont_name=$(".name_lcd").last().attr("name");
            var cont_value = $(".name_lcd").last().attr("value");
            var cont_num=parseInt(cont_name)*parseInt(cont_value);
            if(cont_name==null||cont_name==undefined||cont_name==""){
                cont=0;
                td_cont=0;
            }else {
                var tb_com=$("#tb").find("td:first").attr('id');
                    tb_com=tb_com.replace(/[^0-9]/ig,"");
                    cont=parseInt(tb_com);
                    cont=cont+(cont_num);
                    td_cont=cont;
            }
            for (var i = 0; i < input_row.value; i++) {
                var tableData = "<tr style='border: 1px #3fe6f7 solid'>";
                for (var j = 0; j < input_col.value; j++) {
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

            var namele=$("input[name='led_lcd']:checked").val();
            if(namele==null||namele==""||namele==undefined){
                alert("请选择LED或LCD");
            }else {
                $("#pingMu,#ping-dis,.bod-name").show("clip");
                //console.log("选中一个"+namele);

                $("#pingMu-cancel").off('click').on("click",function () {
                    $("#pingMu,#ping-dis").hide("clip");
                    $(this).css("background","#c9cc19");
                    var _this = this;
                    setTimeout(function(){
                        $(_this).css("background","#00bcd4");
                    },200);
                });
                $("#pingMu-button").off('click').on("click",function () {
                    var pin_name=document.getElementById("pingmu-name").value.trim();
                    var pin_xin="";
                    $("#div-pin .ping").each(function () {
                        pin_xin=$(this).text().trim();
                        if (pin_xin==pin_name){
                            $("#p-name").html("屏幕组名称重复");
                            $("#p-name").fadeIn(1000);
                            $("#p-name").fadeOut(1000);
                            return false;
                        }
                    });
                    if(pin_name==""||pin_name==null){
                        $("#p-name").html("请输入屏幕分组名称");
                        $("#p-name").fadeIn(1000);
                        $("#p-name").fadeOut(1000);
                        return false;
                    }
                    if(pin_xin!=pin_name){
                        $("#tb").html(tb_row);
                        //传输幕墙指令
                        dev_row =input_row.value;
                        dev_col =input_col.value;
                        setJointSet();
                        var tr_name=document.getElementById("pin").value.trim();
                        $(ping_this).text(tr_name);
                        $("#pingMu,#ping-dis").hide("clip");
                        $(".cfposk").hide("drop");
                        $("#cfpo-ent1").hide("drop");

                        $(this).css("background","#c9cc19");
                        var _this = this;
                        setTimeout(function(){
                            $(_this).css("background","#00bcd4");
                        },200);

                        var leng=$("#div-pin div").length;
                        if(leng==null||leng==undefined||leng==""){
                            leng=0;
                        }
                        var pen_div="";
                        var tb_com=$("#tb").find("td:first").attr('id');
                            tb_com=tb_com.replace(/[^0-9]/ig,"");
                            beginId=parseInt(tb_com);
                            row=input_row.value;
                            col=input_col.value;
                            endId=beginId+((row*col)-1);
                            groupName=pin_name;
                            groupId=leng;
                        if(namele=="LED"){
                            pen_div="<div id='ping"+leng+"' name='"+row+"' value='"+col+"' class='col-md-4 ping pm'>"+pin_name+""
                            +"<span class='wen-ip' name='"+beginId+"' value='"+endId+"'>"+namele+"</span>"+
                            +"</div>";
                        }else if (namele=="LCD"){
                            pen_div="<div id='ping"+leng+"' name='"+row+"' value='"+col+"' class='col-md-4 ping pm name_lcd'>"+pin_name+""
                            +"<span class='wen-ip' name='"+beginId+"' value='"+endId+"'>"+namele+"</span>"+
                            +"</div>";
                        }
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
                        console.log(groupId+"::::::::"+groupName+"::::::::"+beginId+":::::"+endId+":::::"+row+"::::"+col);
                        setSetScreenGroup();
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
                                td_cont=cont;
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
                                setJointSet();
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
                                        groupdelete=$(s_longTapBox).attr('id');
                                        groupdelete=groupdelete.replace(/[^0-9]/ig,"");
                                        setDelScreenGroup();
                                        evt.preventDefault();
                                    }
                                });
                            })(s_longTapBox[i]);
                        }
                    }
                });
                $(this).css("background","#c9cc19");
                var _this = this;
                setTimeout(function(){
                    $(_this).css("background","#00bcd4");
                },200);
            }
        });


    }
});

var ping_this="";
$("#div-pin").delegate(".ping","click",function () {
    if($("#copor1").hasClass('copor1-yin')) {
        $("#copor1").removeClass('copor1-yin').addClass('copor1-mei');
    }else{
        $("#pingMu,#ping-dis,.bod-name,#e-c").hide();
        $(".p-name,#div-p-name").show();
        $(".cfposk").show("drop");
        $("#cfpo-ent1").show("drop");
        var tree_name=$(this).clone().children().remove().end().text();
        ping_this=this;
        document.getElementById("pin").value=tree_name;
        $("#subm1").off('click').on("click",function () {
            var pin_name=document.getElementById("pin");
            if(pin_name.value==""){
                alert("请输入屏幕分组名称");
            }else {
                var tree_xin="";
                var sd_name=document.getElementById("pin").value.trim();
                $(".ping").each(function () {
                    tree_xin=$(this).text().trim();
                    if (tree_xin==sd_name){
                        $("#p-name").html("屏幕组名称重复");
                        $("#p-name").fadeIn(1000);
                        $("#p-name").fadeOut(1000);
                        return false;
                    }
                });
                if (sd_name==""||sd_name==null){
                    $("#p-name").html("屏幕组名称不能为空");
                    $("#p-name").fadeIn(1000);
                    $("#p-name").fadeOut(1000);
                    return false;
                }
                if(tree_xin!=sd_name||tree_name==sd_name){//tree_name==sd_name
                    var tr_name=document.getElementById("pin").value.trim();
                    $(ping_this).contents()[0].nodeValue=tr_name;
                    modifyscreengroupName=tr_name;
                    modifyscreengroupId=$(ping_this).attr('id');
                    modifyscreengroupId=modifyscreengroupId.replace(/[^0-9]/ig,"");
                    setModifyScreenGroup();
                    dev_row =document.getElementById("input_row").value.trim();
                    dev_col =document.getElementById("input_col").value.trim();
                    $(ping_this).attr("name",dev_row);
                    $(ping_this).attr("value",dev_col);
                    $(".cfposk").hide("drop");
                    $("#cfpo-ent1").hide("drop");
                }
                $(this).css("color","#c9cc19");
                 var _this = this;
                setTimeout(function(){
                    $(_this).css("color","#fff");
                },200);
                $(this).css("background","#c9cc19");
                var _this = this;
                setTimeout(function(){
                    $(_this).css("background","#00bcd4");
                },200);
            }
        });
    }
});

var penss=0;
var sourceElement="",W="",H="",X="",Y="",kc_id="",w="",h="",tree_xin="",src_val="",pip_W="",pip_H="",pip_X="",pip_Y="",pip_kc_id="",pip_w="",pip_h="",pip_tree_xin="",win_id="",wen="",he="",that="",pip_close="";
$().ready(function(e) {
    var wi=($('#bod-tree .tree-xin,#div-swp .nav-imgs').width());
    var heig=($('#bod-tree .tree-xin,#div-swp .nav-imgs').height());
    $(".wrap").hide();
    $('#bod-tree .tree-xin,#div-swp .nav-imgs').draggable({
        helper:"clone",
        cursor: "move",
        scroll:false,
        appendTo:'body',
        scope : 'pen',
        start: function( event, ui ) {
            $(ui.helper).css({
                width:wi+'px',
                height:heig+'px',
                zIndex:0
            });
            $(this).css("border","2px #c9cc19 solid");
            var _this = this;
            setTimeout(function(){
                $(_this).css("border","1px #1acbfc solid");

            },300);
        },
        stop:function (event,ui) {
            $("#mqxians .nav-imgs").removeClass('animation');
        }
    });
    $('#div-s-sho').droppable({
        scope : 'pen',
        drop: function( event, ui ) {
            var ud=ui.draggable.clone();
            var n="";
            var x_n=ud.clone().children().remove().end().text().trim();
            $(".col-md-4").each(function () {
                n=$(this).clone().children().remove().end().text().trim();
                if (n==x_n){
                    alert("已收藏");
                    return false;
                }
            });
            if (n!=x_n){
                ud.appendTo($("#div-xin")).css({
                    width:'22%',
                    height: '50%',
                    border: '1px salmon solid',
                    background: '#2e6da4',
                    marginBottom: '3%',
                    marginRight: '3%',
                    opacity:1,
                    position: 'relative',
                });
                ud.addClass("col-md-4");
                ud.draggable({
                    helper:"clone",
                    cursor: "move",
                    revert : 'invalid',
                    scroll:false,
                    appendTo:'body',
                    scope : 'pen',
                    start: function( event, ui ) {
                        $(ui.helper).css({
                            width:wi+'px',
                            height:heig+'px',
                            zIndex:13,
                        });
                        $("#xin-dis").css("display","none");
                        $("#xin-hao").hide({ effect: "slide", direction: "left " });
                    },
                    stop: function (event, ui) {
                        $("#xin-dis").css("display","block");
                        $("#xin-hao").show({ effect: "slide", direction: "right" });
                    }
                });
                var s_longTapBox = ud;
                for (var i=0;i<s_longTapBox.length;i++) {
                    (function(s_longTapBox) {
                        Transform(s_longTapBox);
                        new AlloyFinger(s_longTapBox, {
                            longTap:function(evt){
                                alert("nichanganlema?");
                                $("#xin_name").fadeIn(500);
                                $("#xin_name").fadeOut(500);
                                $(s_longTapBox).remove();
                                evt.preventDefault();
                            }
                        });
                    })(s_longTapBox[i]);
                }
            }
        }
    });

    var timeoutDebounce = null;
    /**
    callbackFunc 回调函数
    waitDuration 到计时频率，毫秒级；比如：1000 代表1秒
    */
    var doubleTapDebounce = function(doublebackFunc,waitDuration){
        clearTimeout(timeoutDebounce);
        timeoutDebounce=setTimeout(function(){
            doublebackFunc();
        },waitDuration);
    };

    // 存储webglPlayer播放器
    //var webglPlayers2 = [];
    $('td[id^="com"]').livequery(function() {
        $(this).droppable({
            scope : 'pen',
            drop: function (event, ui) {
                wen=parseInt(($('td[id^="com"]').width())+3);
                he=parseInt(($('td[id^="com"]').height())+3);
                sourceElement = $(ui.helper.context).attr("id");
                var source = ui.draggable.clone();
                //var canvas = (source.children("canvas").eq(0))[0];
                // TODO
                // 多路回显
                // let players = wsWebglPlayersMap.get(canvas);
                // players.push(new WebGLPlayer(canvas, {
                //     preserveDrawingBuffer: false
                // }));
                // wsWebglPlayersMap.put(canvas, players);
                // 多路回显

                // 每次拖动生成一个保存
                //webglPlayers2.push(new WebGLPlayer(canvas, {
                    //preserveDrawingBuffer: false
                //}));
                $(source).removeClass('seni');
                $('<img/>',{
                    src: '../image/叉叉.png',
                    click:function() {
                        pip_close = source.attr("id");
                        $(source).remove();
                        setPipWidgetClose();
                    }
                }).css({
                    width:'23px',
                    height:'23px',
                    position: 'absolute',
                    top: '0%',
                    right: '0%',
                }).appendTo(source);
                source.appendTo($(this));
                source.css({
                    width:wen+'px',
                    height:he+'px',
                    top:"0",
                    zIndex:0,
                    fontSize:'1vw',
                    position:'absolute',
                    opacity:1,
                    left:'0',
                    marginBottom: '0',
                    marginRight: '0',
                    border: '1px solid #1acbfc',
                }).draggable({
                    revert : 'invalid',
                    scope : 'drop',
                    containment: '#mqxians',
                    //grid: [wen, he],
                }).resizable({
                    containment: '#mqxians',
                    handles: "all",
                    stop:function (event,ui) {
                        var uiu=$(ui.helper);
                        var uiu_w=$(uiu).width();
                        var uiu_h=$(uiu).height();
                        var uiu_l=$(uiu).offset().left-($('#mqxians').offset().left);
                        var uiu_t=$(uiu).offset().top-($('#mqxians').offset().top);
                        $(uiu).appendTo($("#mqxians")).css({"width": uiu_w,"height": uiu_h,"top": uiu_t,"left": uiu_l});
                        pip_tree_xin=$(uiu).attr("id");
                        pip_W=$(uiu).width();
                        pip_H=$(uiu).height();
                        pip_X=$(uiu).offset().left;
                        pip_Y=$(uiu).offset().top;
                        pip_kc_id=$(uiu).find(' input').attr('id');
                        pip_w=$(uiu).find(' input').attr('name');
                        pip_h=$(uiu).find(' input').attr('class');
                        src_val=$(uiu).find(' input').attr('value');
                        console.log("一号："+pip_tree_xin+"pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                        setPipWidgetChange();
                    },
                });
                tree_xin=$(this);
                W=$(this).find('.tree-xin,.nav-imgs').outerWidth()+1;
                H=$(this).find('.tree-xin,.nav-imgs').outerHeight()+1;
                X=$(this).find('.tree-xin,.nav-imgs').offset().left-1;
                Y=$(this).find('.tree-xin,.nav-imgs').offset().top-1;
                kc_id = $(this).find(' input').attr('id');
                w = $(this).find(' input').attr('name');
                h = $(this).find(' input').attr('class');
                var id_tree_s=$(this).find('.tree-xin,.nav-imgs');
                console.log("W::"+W+":H::"+H+":X::"+X+":Y::"+Y+":ID::"+kc_id);
                setNetBasicInfo();
                var objW=$(".wrap").width();
                var objH=$(".wrap").height();
                source.mousedown(function(e) {
                    if (3 == e.which) {
                        var selfX=objW+e.pageX;
                        var selfY=objH+e.pageY;
                        var bodyW=document.documentElement.clientWidth+document.documentElement.scrollLeft;
                        var bodyH=document.documentElement.clientHeight+document.documentElement.scrollTop;
                        if(selfX>bodyW && selfY>bodyH){
                            $(".wrap").css({top:(bodyH-objH),left:(bodyW-objW)}).show();
                        }else if(selfY>bodyH){
                            $(".wrap").css({top:(bodyH-objH),left:e.pageX}).show();
                        }else if(selfX>bodyW){
                            $(".wrap").css({top:e.pageY,left:(bodyW-objW)}).show();
                        }else{
                            $(".wrap").css({top:e.pageY,left:e.pageX}).show();
                        }
                        $(".wrap li").hover(function(){
                            $(this).addClass("current");
                        },function(){
                            $(this).removeClass("current");
                        }).click(function(){
                            pip_close = source.attr("id");
                            id_tree_s.remove();
                            $(this).parent().parent().hide();
                            setPipWidgetClose();
                        });
                    }
                });
                var timerProvinceClick=null;
                $("#mqxians .tree-xin,#mqxians .nav-imgs").off('click').click(function () {
                    clearTimeout(timerProvinceClick);
                    var maxZi = Math.max.apply(null,
                        $.map($('#mqxians .tree-xin,#mqxians .nav-imgs'), function(e,n) {
                            if ($(e).css('position') != 'static')
                            return parseInt($(e).css('z-index')) || -1;
                        })
                    );
                    var zind=maxZi+1;
                    time = setTimeout(function() {
                        $(this).appendTo($("#mqxians")).css({"z-index": zind + 1});
                        pip_tree_xin=$(this).attr("id");
                        pip_W=$(this).width();
                        pip_H=$(this).height();
                        pip_X=$(this).offset().left;
                        pip_Y=$(this).offset().top;
                        pip_kc_id=$(this).find(' input').attr('id');
                        pip_w=$(this).find(' input').attr('name');
                        pip_h=$(this).find(' input').attr('class');
                        src_val=$(this).find(' input').attr('value');
                        console.log("11111号：pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                        setPipWidgetChange();
                    },300);
                });
                var maxZ = Math.max.apply(null,
                    $.map($('#mqxians .tree-xin,#mqxians .nav-imgs'), function(e,n) {
                        if ($(e).css('position') != 'static')
                        return parseInt($(e).css('z-index')) || -1;
                    })
                );
                var dbflag=true;
                var _dbww,_dbhh,_dblef,_dbto;
                _dbww=$(source).width();
                _dbhh=$(source).height();
                _dblef=$(source).offset().left-($('#mqxians').offset().left);
                _dbto=$(source).offset().top-($('#mqxians').offset().top);
                var ziz=maxZ+1;
                source.off('dblclick').on("dblclick", function () {
                    clearTimeout(timerProvinceClick);
                    if(dbflag){
                        dbflag = false;
                        source.appendTo($("#mqxians")).css({
                            top: '0',
                            left:'0',
                            width: '100%',
                            height: '100%',
                            zIndex:ziz
                        });
                        pip_tree_xin=source.attr("id");
                        pip_W=source.width();
                        pip_H=source.height();
                        pip_X=source.offset().left+1;
                        pip_Y=source.offset().top+1;
                        pip_kc_id=source.find(' input').attr('id');
                        pip_w=source.find(' input').attr('name');
                        pip_h=source.find(' input').attr('class');
                        src_val=source.find(' input').attr('value');
                        var div1 = $('td[id^="com"]').outerWidth()+1;
                        var div2 = $('td[id^="com"]').outerHeight()+1;
                        dev_row = document.getElementById("input_row").value.trim();
                        dev_col = document.getElementById("input_col").value.trim();
                        var div_1c=div1*dev_col;
                        var div_2r=div2*dev_row;
                        if(pip_W!=div_1c){
                            pip_W=div_1c;
                        }
                        if (pip_H!=div_2r){
                            pip_H=div_2r;
                        }
                        console.log("22222号：pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                        setPipWidgetChange();
                    }else {
                        dbflag=true;
                        source.appendTo($("#mqxians")).css({"width": _dbww,"height": _dbhh,"top": _dbto,"left": _dblef,"z-index":ziz});
                        pip_tree_xin=source.attr("id");
                        pip_W=source.outerWidth()+2;
                        pip_H=source.outerHeight()+2;
                        pip_X=source.offset().left-1;
                        pip_Y=source.offset().top-1;
                        pip_kc_id=source.find(' input').attr('id');
                        pip_w=source.find(' input').attr('name');
                        pip_h=source.find(' input').attr('class');
                        src_val=source.find(' input').attr('value');
                        console.log("33333号：pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                        setPipWidgetChange();
                    }
                });
                //移动端双击放大·缩小
                var doubleTapImg = source;
                for (var i = 0; i < doubleTapImg.length; i++) {
                    (function (doubleTapImg) {
                        var flag = true;
                        var _ww, _hh, _lef, _to;
                        _ww = $(doubleTapImg).outerWidth();
                        _hh = $(doubleTapImg).outerHeight();
                        _lef = $(doubleTapImg).offset().left - ($('#mqxians').offset().left);
                        _to = $(doubleTapImg).offset().top - ($('#mqxians').offset().top);
                        var xw = $('#mqxians').width();
                        var xh = $('#mqxians').height();
                        Transform(doubleTapImg);
                        new AlloyFinger(doubleTapImg, {
                            doubleTap: function (evt) {
                                clearTimeout(timerProvinceClick);
                                var maxZ = Math.max.apply(null,
                                    $.map($('#mqxians .tree-xin,#mqxians .nav-imgs'), function (e, n) {
                                        if ($(e).css('position') != 'static')
                                            return parseInt($(e).css('z-index')) || -1;
                                    })
                                );
                                var zin = maxZ + 1;
                                doubleTapDebounce(function(){
                                    if (flag) {
                                        flag = false;
                                        $(doubleTapImg).appendTo($("#mqxians")).css({"width": '100%', "height": '100%', "top": "0px", "left": "0px", "z-index": zin});
                                        pip_tree_xin = $(doubleTapImg).attr("id");
                                        pip_W = $(doubleTapImg).width();
                                        pip_H = $(doubleTapImg).height();
                                        pip_X = $(doubleTapImg).offset().left+1;
                                        pip_Y = $(doubleTapImg).offset().top+1;
                                        pip_kc_id = $(doubleTapImg).find(' input').attr('id');
                                        pip_w = $(doubleTapImg).find(' input').attr('name');
                                        pip_h = $(doubleTapImg).find(' input').attr('class');
                                        src_val = $(doubleTapImg).find(' input').attr('value');
                                        var div1 = $('td[id^="com"]').outerWidth()+1;
                                        var div2 = $('td[id^="com"]').outerHeight()+1;
                                        dev_row = document.getElementById("input_row").value.trim();
                                        dev_col = document.getElementById("input_col").value.trim();
                                        var div_1c=div1*dev_col;
                                        var div_2r=div2*dev_row;
                                        if(pip_W!=div_1c){
                                            pip_W=div_1c;
                                        }
                                        if (pip_H!=div_2r){
                                            pip_H=div_2r;
                                        }
                                        console.log("44444号：pip_W::" + pip_W + ":pip_H::" + pip_H + ":pip_X::" + pip_X + ":pip_Y::" + pip_Y + ":ID::" + pip_kc_id + ":W:" + pip_w + ":H:" + pip_h);
                                        setPipWidgetChange();
                                        evt.preventDefault();
                                    } else {
                                        flag = true;
                                        $(doubleTapImg).appendTo($("#mqxians")).css({"width": _ww,"height": _hh,"top": _to,"left": _lef,"z-index": zin});
                                        pip_tree_xin = $(doubleTapImg).attr("id");
                                        pip_W = $(doubleTapImg).outerWidth() + 2;
                                        pip_H = $(doubleTapImg).outerHeight() + 2;
                                        pip_X = $(doubleTapImg).offset().left-1;
                                        pip_Y = $(doubleTapImg).offset().top-1;
                                        pip_kc_id = $(doubleTapImg).find(' input').attr('id');
                                        pip_w = $(doubleTapImg).find(' input').attr('name');
                                        pip_h = $(doubleTapImg).find(' input').attr('class');
                                        src_val = $(doubleTapImg).find(' input').attr('value');
                                        console.log("55555号：pip_W::" + pip_W + ":pip_H::" + pip_H + ":pip_X::" + pip_X + ":pip_Y::" + pip_Y + ":ID::" + pip_kc_id + ":W:" + pip_w + ":H:" + pip_h);
                                        setPipWidgetChange();
                                    }
                                },200);
                            }
                        });
                    })(doubleTapImg[i]);
                }
                //移动端双指放大·缩小
                /*var pinchImg = source;
                for (var i = 0; i < pinchImg.length; i++) {
                    (function (pinchImg) {
                        Transform(pinchImg);
                        var initScale = 1;
                        new AlloyFinger(pinchImg, {
                            multipointStart: function () {
                                initScale = pinchImg.scaleX;
                            },
                            pinch: function (evt) {
                                pinchImg.scaleX = pinchImg.scaleY = initScale * evt.zoom;
                            }
                        });
                    })(pinchImg[i]);
                }*/
                var longTapBox = source;
                for (var i=0;i<longTapBox.length;i++) {
                    (function(longTapBox) {
                        Transform(longTapBox);
                        new AlloyFinger(longTapBox, {
                            longTap:function(evt){
                                clearTimeout(timerProvinceClick);
                                var _left=$(longTapBox).offset().left;
                                var _top=$(longTapBox).offset().top;
                                var div1 = $(longTapBox).width();
                                var div2 = $(longTapBox).height();
                                $(longTapBox).appendTo($(".body")).css({"left":_left,"top":_top,"opacity": 0.7,"width": div1,"height": div2});
                                $("#del-shan").show();
                                $(longTapBox).draggable("option","scope","shan");
                                evt.preventDefault();
                            },
                            pressMove:function(evt){
                                clearTimeout(timerProvinceClick);
                                $("#del-shan").droppable({
                                    scope : 'shan',
                                    tolerance : "touch",
                                    drop : function(event, ui) {
                                        var udc = ui.draggable;
                                        //$(udc).css("background","#6f2525");
                                        pip_close=udc.attr("id");
                                        $(udc).remove();
                                        $("#del-shan").hide();
                                        setPipWidgetClose();
                                    }
                                });
                                evt.preventDefault();
                            },
                            touchEnd:function (evt) {
                                clearTimeout(timerProvinceClick);
                                $(longTapBox).css("opacity",1);
                                $("#del-shan").hide();
                                $(longTapBox).draggable("option","scope","drop");
                                evt.preventDefault();
                            }
                        });
                    })(longTapBox[i]);
                }
            },
        });
    });

    $("#mqxians").droppable({
        scope : 'drop',
        drop : function(event, ui) {
            var ud = ui.draggable;
            $(ud).removeClass('seni');
            var bgColor="";
            var colorArray =new Array("0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f");
            for(var i=0; i<6; i++){
                bgColor +=colorArray[Math.floor(Math.random()*16)];
            }
            var zcx=parseInt($(ud).siblings().last().css("z-index"));
            var _timerProvinceClick=null;
            var zi=zcx+1;
            ud.appendTo($("#mqxians")).css({
                position : 'absolute',
                left : ui.offset.left-($('#mqxians').offset().left),
                top : ui.offset.top-($('#mqxians').offset().top),
                border: '2px solid #'+bgColor,
                zIndex:zi
            }).draggable({
                scope : 'drop',
                revert : 'invalid',
                containment: 'parent',
                stop: function(event, ui) {
                    var timeOutEvent=0;
                    $("#mqxians .tree-xin,#mqxians .nav-imgs").off('click').click(function () {
                        var maxZ = Math.max.apply(null,
                            $.map($('#mqxians .tree-xin,#mqxians .nav-imgs'), function(e,n) {
                                if ($(e).css('position') != 'static')
                                return parseInt($(e).css('z-index')) || -1;
                            })
                        );
                        var zind=maxZ+1;
                        $(this).css({"z-index":zind+1});
                        pip_tree_xin=$(this).attr("id");
                        pip_W = $(this).outerWidth()+1;
                        pip_H = $(this).outerHeight()-1;
                        pip_X = $(this).offset().left+1;
                        pip_Y = $(this).offset().top+1;
                        pip_kc_id=$(this).find(' input').attr('id');
                        pip_w=$(this).find(' input').attr('name');
                        pip_h=$(this).find(' input').attr('class');
                        src_val=$(this).find(' input').attr('value');
                        console.log("66666号：pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                        setPipWidgetChange();
                    });
                    $(event.originalEvent.target).on({
                        touchend: function(e){
                            var _value=$(ui.helper.context).find(" input").attr("value");
                            if (_value==""||_value==null || _value == undefined){
                                console.log("当前元素");
                                var ipc_music="";
                                var tree_this=$(ud);
                                $( "#slider-vertical" ).slider({
                                    orientation: "vertical",
                                    range: "min",
                                    min: 0,
                                    max: 100,
                                    value: 30,
                                    start: function(event, ui) {
                                        $("#sliderValue").show();
                                    },
                                    stop: function (event,ui) {
                                        $("#sliderValue").fadeOut(300);
                                    },
                                    stop: function (event,ui) {
                                         ipc_music=ui.value;
                                         $(tree_this).find(" input").val($(tree_this).find(" input").val() + (ipc_music-1));
                                         $("#sliderValue").fadeOut(300);
                                    }
                                });
                            }else if(_value!=""){
                                console.log("当前元素:"+$(ui.helper.context).find(" input").attr("value"));
                                var as=$(ui.helper.context).find(" input");
                                var ipc_music="";
                                var tree_this=$(ud);
                                $( "#slider-vertical" ).slider({
                                    orientation: "vertical",
                                    range: "min",
                                    min: 0,
                                    max: 100,
                                    value: _value,
                                    start: function(event, ui) {
                                        $("#sliderValue").show();
                                    },
                                    stop: function (event,ui) {
                                        $("#sliderValue").fadeOut(300);
                                    },
                                    stop: function (event,ui) {
                                        ipc_music=ui.value;
                                        as.attr("value","");
                                        $(tree_this).find(" input").val($(tree_this).find(" input").val() + (ipc_music-1));
                                        $("#sliderValue").fadeOut(300);

                                    }
                                });
                            }
                        }
                    });
                    //event.stopImmediatePropagation();
                }
            }).resizable({
                containment: 'parent',
                handles: "all",
                stop:function (event,ui) {
                    pip_tree_xin=$(this).attr("id");
                    pip_W=$(this).width();
                    pip_H=$(this).height();
                    pip_X=$(this).offset().left;
                    pip_Y=$(this).offset().top;
                    pip_kc_id=$(this).find(' input').attr('id');
                    pip_w=$(this).find(' input').attr('name');
                    pip_h=$(this).find(' input').attr('class');
                    src_val=$(this).find(' input').attr('value');
                    console.log("二号：pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                    setPipWidgetChange();
                },
            });
            pip_tree_xin=ud.attr("id");
            pip_W=$(ud).outerWidth()+2;
            pip_H=$(ud).outerHeight()+2;
            pip_X=ud.offset().left;
            pip_Y=ud.offset().top;
            pip_kc_id=ud.find(' input').attr('id');
            pip_w=ud.find(' input').attr('name');
            pip_h=ud.find(' input').attr('class');
            src_val=ud.find(' input').attr('value');
            console.log("三号：当前窗口::"+pip_tree_xin+"pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
            setPipWidgetChange();
            var objW=$(".wrap").width();
            var objH=$(".wrap").height();
            ud.mousedown(function (e) {
                if (3 == e.which) {
                    var selfX=objW+e.pageX;
                    var selfY=objH+e.pageY;
                    var bodyW=document.documentElement.clientWidth+document.documentElement.scrollLeft;
                    var bodyH=document.documentElement.clientHeight+document.documentElement.scrollTop;
                    if(selfX>bodyW && selfY>bodyH){
                        $(".wrap").css({top:(bodyH-objH),left:(bodyW-objW)}).show();
                    }else if(selfY>bodyH){
                        $(".wrap").css({top:(bodyH-objH),left:e.pageX}).show();
                    }else if(selfX>bodyW){
                        $(".wrap").css({top:e.pageY,left:(bodyW-objW)}).show();
                    }else{
                        $(".wrap").css({top:e.pageY,left:e.pageX}).show();
                    }
                    $(".wrap li").hover(function(){
                        $(this).addClass("current");
                    },function(){
                        $(this).removeClass("current");
                    }).click(function(){
                        pip_close = ud.attr("id");
                        ui.draggable.remove();
                        $(this).parent().parent().hide();
                        setPipWidgetClose();
                    });
                }
            });
            var _dbflag = true;
            var _dbww,_dbhh,_dblef,_dbto;
            _dbww=$(ud).width();
            _dbhh=$(ud).height();
            _dblef=$(ud).offset().left-($('#mqxians').offset().left);
            _dbto=$(ud).offset().top-($('#mqxians').offset().top);
            ud.off('dblclick').on("dblclick", function () {
                clearTimeout(_timerProvinceClick);
                if(_dbflag){
                    _dbflag = false;
                    ud.css({
                        top: '0px',
                        left:'0px',
                        width: '100%',
                        height: '100%',
                        zIndex:zi
                    });
                    pip_tree_xin=ud.attr("id");
                    pip_W=ud.width();
                    pip_H=ud.height();
                    pip_X=ud.offset().left+1;
                    pip_Y=ud.offset().top+1;
                    pip_kc_id=ud.find(' input').attr('id');
                    pip_w=ud.find(' input').attr('name');
                    pip_h=ud.find(' input').attr('class');
                    src_val=ud.find(' input').attr('value');
                    src_val = $(doubleTapImg).find(' input').attr('value');
                    var div1 = $('td[id^="com"]').outerWidth()+1;
                    var div2 = $('td[id^="com"]').outerHeight()+1;
                    dev_row = document.getElementById("input_row").value.trim();
                    dev_col = document.getElementById("input_col").value.trim();
                    var div_1c=div1*dev_col;
                    var div_2r=div2*dev_row;
                    if(pip_W!=div_1c){
                        pip_W=div_1c;
                    }
                    if (pip_H!=div_2r){
                        pip_H=div_2r;
                    }
                    console.log("77777号：当前窗口::"+pip_tree_xin+"pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                    setPipWidgetChange();
                }else {
                    _dbflag = true;
                    $(ud).css({"width": _dbww,"height": _dbhh,"top": _dbto,"left": _dblef,"z-index":zi});
                    pip_tree_xin=ud.attr("id");
                    pip_W=ud.outerWidth()+2;
                    pip_H=ud.outerHeight()+2;
                    pip_X=ud.offset().left;
                    pip_Y=ud.offset().top;
                    pip_kc_id=ud.find(' input').attr('id');
                    pip_w=ud.find(' input').attr('name');
                    pip_h=ud.find(' input').attr('class');
                    src_val=ud.find(' input').attr('value');
                    console.log("88888号：pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                    setPipWidgetChange();
                }
            });
            var doubleTapImg=ud.get(0);
            var _flag = true;
            var _ww, _hh, _lef, _to;
            _ww = $(doubleTapImg).outerWidth();
            _hh = $(doubleTapImg).outerHeight();
            _lef = $(doubleTapImg).offset().left - ($('#mqxians').offset().left);
            _to = $(doubleTapImg).offset().top - ($('#mqxians').offset().top);
            Transform(doubleTapImg);
            new AlloyFinger(doubleTapImg, {
                doubleTap: function (evt) {
                    var zz = parseInt($(doubleTapImg).siblings().last().css("z-index"));
                    var zin = zz + 1;
                    doubleTapDebounce(function(){
                        if (_flag) {
                            _flag = false;
                            $(doubleTapImg).css({"width": '100%', "height": '100%', "top": "0px", "left": "0px", "z-index": zin});
                            pip_tree_xin = $(doubleTapImg).attr("id");
                            pip_W = $(doubleTapImg).width();
                            pip_H = $(doubleTapImg).height();
                            pip_X = $(doubleTapImg).offset().left+1;
                            pip_Y = $(doubleTapImg).offset().top+1;
                            pip_kc_id = $(doubleTapImg).find(' input').attr('id');
                            pip_w = $(doubleTapImg).find(' input').attr('name');
                            pip_h = $(doubleTapImg).find(' input').attr('class');
                            src_val = $(doubleTapImg).find(' input').attr('value');
                            var div1 = $('td[id^="com"]').outerWidth()+1;
                            var div2 = $('td[id^="com"]').outerHeight()+1;
                            dev_row = document.getElementById("input_row").value.trim();
                            dev_col = document.getElementById("input_col").value.trim();
                            var div_1c=div1*dev_col;
                            var div_2r=div2*dev_row;
                            if(pip_W!=div_1c){
                                pip_W=div_1c;
                            }
                            if (pip_H!=div_2r){
                                pip_H=div_2r;
                            }
                            console.log("99999号：pip_W::" + pip_W + ":pip_H::" + pip_H + ":pip_X::" + pip_X + ":pip_Y::" + pip_Y + ":ID::" + pip_kc_id + ":W:" + pip_w + ":H:" + pip_h);
                            setPipWidgetChange();
                        } else {
                            _flag = true;
                            $(doubleTapImg).css({"width": _ww, "height": _hh, "top": _to, "left": _lef, "z-index": zin});
                            pip_tree_xin = $(doubleTapImg).attr("id");
                            pip_W = $(doubleTapImg).outerWidth()+2;
                            pip_H = $(doubleTapImg).outerHeight()+2;
                            pip_X = $(doubleTapImg).offset().left;
                            pip_Y = $(doubleTapImg).offset().top;
                            pip_kc_id = $(doubleTapImg).find(' input').attr('id');
                            pip_w = $(doubleTapImg).find(' input').attr('name');
                            pip_h = $(doubleTapImg).find(' input').attr('class');
                            src_val = $(doubleTapImg).find(' input').attr('value');
                            console.log("00000号：pip_W::" + pip_W + ":pip_H::" + pip_H + ":pip_X::" + pip_X + ":pip_Y::" + pip_Y + ":ID::" + pip_kc_id + ":W:" + pip_w + ":H:" + pip_h);
                            setPipWidgetChange();
                            evt.preventDefault();
                        }
                    },200);
                },
            });

            //移动端双指放大·缩小
            /*var pinchImg = ud;
            for (var i = 0; i < pinchImg.length; i++) {
                (function (pinchImg) {
                    Transform(pinchImg);
                    var initScale = 1;
                        new AlloyFinger(pinchImg, {
                            multipointStart: function () {
                                    if (pinchImg.scaleX < 1) {
                                        pinchImg.scaleX = pinchImg.scaleY = 2;
                                    }else if(pinchImg.scaleX>2){
                                        pinchImg.scaleX = pinchImg.scaleY = 2;
                                    }
                                    initScale=el.scaleX;
                            },
                            pinch: function (evt) {
                                pinchImg.scaleX = pinchImg.scaleY = initScale * evt.zoom;
                            }
                        });
                })(pinchImg[i]);
            }*/
            var longTapBox = ud;
            for (var i=0;i<longTapBox.length;i++) {
                (function(longTapBox) {
                    Transform(longTapBox);
                    new AlloyFinger(longTapBox, {
                        longTap:function(evt){
                        clearTimeout(_timerProvinceClick);
                            var _left=$(longTapBox).offset().left;
                            var _top=$(longTapBox).offset().top;
                            var div1 = $(longTapBox).width();
                            var div2 = $(longTapBox).height();
                            $(longTapBox).appendTo($(".body")).css({"left":_left,"top":_top,"opacity": 0.7,"width": div1,"height": div2});
                            $("#del-shan").show();
                            evt.preventDefault();
                        },
                        pressMove:function(evt){
                            clearTimeout(_timerProvinceClick);
                            $(longTapBox).draggable("option","containment","#mqxians");
                            $("#del-shan").droppable({
                                scope : 'drop',
                                drop : function(event, ui) {
                                    var udc = ui.draggable;
                                    pip_close=udc.attr("id");
                                    $(udc).remove();
                                    $("#del-shan").hide();
                                    setPipWidgetClose();
                                    console.log("关闭窗口"+pip_close);
                                }
                            });
                            evt.preventDefault();
                        },
                        touchEnd:function (evt) {
                            clearTimeout(_timerProvinceClick);
                            $(longTapBox).css("opacity",1);
                            $("#del-shan").hide();
                            evt.preventDefault();
                        }
                    });
                })(longTapBox[i]);
            }
            var mqxians_Img = ud.get(0);
            var store = {
                scale: 1
            };
            var zwc = 0,zhc = 0,zxc = 0,zyc = 0;
            // 缩放事件的处理
            mqxians_Img.addEventListener('touchstart', function (event) {
                var touches = event.touches;
                var events = touches[0];
                var events2 = touches[1];

                event.preventDefault();

                // 第一个触摸点的坐标
                store.pageX = events.pageX;
                store.pageY = events.pageY;

                store.moveable = true;

                if (events2) {
                    store.pageX2 = events2.pageX;
                    store.pageY2 = events2.pageY;
                }

                store.originScale = store.scale || 1;
            });
            document.addEventListener('touchmove', function (event) {
                if (!store.moveable) {
                    return;
                }

                //event.preventDefault();

                var touches = event.touches;
                var events = touches[0];
                var events2 = touches[1];

                // 双指移动
                if (events2) {

                    // 第2个指头坐标在touchmove时候获取
                    if (!store.pageX2) {
                        store.pageX2 = events2.pageX;
                    }
                    if (!store.pageY2) {
                        store.pageY2 = events2.pageY;
                    }

                    // 获取坐标之间的举例
                    var getDistance = function (start, stop) {
                        return Math.hypot(stop.x - start.x, stop.y - start.y);
                    };

                    // 双指缩放比例计算
                    var zoom = getDistance({
                        x: events.pageX,
                        y: events.pageY
                    }, {
                        x: events2.pageX,
                        y: events2.pageY
                    }) /
                    getDistance({
                        x: store.pageX,
                        y: store.pageY
                    }, {
                        x: store.pageX2,
                        y: store.pageY2
                    });
                    var a_x = $("#a").offset().left;
                    var a_y = $("#a").offset().top;
                    var xx = $("#mqxians").offset().left;
                    var yy = $("#mqxians").offset().top;
                    var ww = $("#mqxians").outerWidth();
                    var hh = $("#mqxians").outerHeight();
                    var ww2 = $(mqxians_Img).width();
                    var hh2 = $(mqxians_Img).height();
                    var xx2 = $(mqxians_Img).offset().left;
                    var yy2 = $(mqxians_Img).offset().top;

                    //元素获取左（上下），右(上下)角的位置
                    var upper_right_X = (xx2 - xx) + ww2;//右上角X
                    var lower_left_Y = (yy2 - yy) + hh2;//左下角Y
                    var upper_left_X = (xx2 - xx);//左上角X
                    var upper_left_Y = (yy2 - yy);//左上角Y
                    if (upper_left_X < 0) {
                        upper_left_X = 0;
                    }
                    if (upper_left_Y < 0) {
                        upper_left_Y = 0;
                    }

                    //获取边界窗口的左（上下），右(上下)角的位置
                    var upper_right_X2 = (xx - a_x) + ww;//右上角X
                    var lower_left_Y2 = (yy - a_y) + hh;//左下角Y
                    var upper_left_X2 = (xx - a_x);//左上角X
                    var upper_left_Y2 = (yy - a_y);//左上角Y
                    if (upper_left_X2 < 0) {
                        upper_left_X2 = 0;
                    }
                    if (upper_left_Y2 < 0) {
                        upper_left_Y2 = 0;
                    }
                    //alert("改变后：X:::" + upper_left_X2 + "Y:::" + upper_left_Y2 + "W:::" + (upper_right_X2-upper_left_X2) + "H:::" + (lower_left_Y2-upper_left_Y2));
                    //alert("改变后：X:::" + upper_left_X + "Y:::" + upper_left_Y + "W:::" + (upper_right_X2-upper_left_X) + "H:::" + (lower_left_Y2-upper_left_Y));
                    //alert(a_x+"屏幕的"+a_y+"XX:"+xx+"YY:"+yy+"WW:"+ww+"HH:"+hh);
                    //alert("窗口的XX2:"+xx2+"YY2:"+yy2+"WW2:"+ww2+"HH2:"+hh2);
                    // 应用在元素上的缩放比例
                    var newScale = store.originScale * zoom;

                    //第一种情况：四边不靠，
                    if (parseInt(upper_right_X) <= parseInt(upper_right_X2) && parseInt(lower_left_Y) <= parseInt(lower_left_Y2) && parseInt(upper_left_X) >= parseInt(upper_left_X2) && parseInt(upper_left_Y) >= parseInt(upper_left_Y2)) {
                        // 最大缩放比例限制
                        if (newScale > 2) {
                            newScale = 2;
                        }
                        // 记住使用的缩放值
                        store.scale = newScale;
                        penss=newScale;
                        // 图像应用缩放效果
                        //mqxians_Img.style.transform = 'scale(' + newScale + ')';
                        //alert(zxc+":1:2:"+zyc+"::::"+zwc+":w:h:"+zhc);//算出transform宽高

                        zxc = ($(mqxians_Img).offset().left)/penss;
                        zyc = ($(mqxians_Img).offset().top)/penss;
                        zwc = ($(mqxians_Img).outerWidth())*penss;
                        zhc = ($(mqxians_Img).outerHeight())*penss;
                        if(zwc>ww){
                            zwc=ww;
                        }
                        if(zhc>hh){
                            zhc=hh;
                        }
                        /*alert("x::"+zxc+":y::"+zyc+":w::"+zwc+":h:::"+zhc+":ww::"+ww+":hh::"+hh+":xx::"+xx+":yy::"+yy);*/
                        mqxians_Img.style.width=zwc+"px";
                        mqxians_Img.style.height=zhc+"px";
                        mqxians_Img.css("left",zxc+"px");
                        mqxians_Img.css("top",zyc+"px");
                    }

                    //第二种：缩小
                    /*if(parseInt(xx) == parseInt($(ud).offset().left) && parseInt(yy) == parseInt($(ud).offset().top) && parseInt(ww) == parseInt($(ud).outerWidth()) && parseInt(hh) == parseInt($(ud).outerHeight())){
                        // 最大缩放比例限制
                        if (newScale < 2) {
                            newScale1 = penss;
                        }
                        // 记住使用的缩放值
                        store.scale = newScale1;
                        penss=newScale1;
                        // 图像应用缩放效果
                        zwc = ($(mqxians_Img).outerWidth())/penss;
                        zhc = ($(mqxians_Img).outerHeight())/penss;
                        zxc = ($(mqxians_Img).offset().left)/penss;
                        zyc = ($(mqxians_Img).offset().top)/penss;

                        mqxians_Img.style.width=zwc+"px";
                        mqxians_Img.style.height=zhc+"px";
                        mqxians_Img.css("left",zxc+"px");
                        mqxians_Img.css("top",zyc+"px");
                    }*/
                    
                    //第三种：只靠左上角，
                    if(parseInt(xx) == parseInt($(ud).offset().left) || parseInt(yy) == parseInt($(ud).offset().top)){
                        // 最大缩放比例限制
                        if (newScale > 2) {
                            newScale = 2;
                        }
                        zwc = ww-parseInt(upper_right_X)-parseInt($(mqxians_Img).outerWidth());
                        zhc = hh-parseInt(lower_left_Y)-parseInt($(mqxians_Img).outerHeight());
                        mqxians_Img.style.width=zwc+"px";
                        mqxians_Img.style.height=zhc+"px";
                    }

                    //第四种：只靠右上角，
                    /*// 最大缩放比例限制
                        if (newScale > 2) {
                            newScale = 2;
                        }
                        zwc = ww-parseInt(upper_right_X)-parseInt($(mqxians_Img).outerWidth());
                        zhc = hh-parseInt(lower_left_Y)-parseInt($(mqxians_Img).outerHeight());
                        mqxians_Img.style.width=zwc+"px";
                        mqxians_Img.style.height=zhc+"px";
                    }*/

                    //第五种：只靠左下角，
                    /*if(parseInt(xx) == parseInt($(ud).offset().left) || parseInt(yy) == parseInt($(ud).offset().top)){
                        // 最大缩放比例限制
                        if (newScale > 2) {
                            newScale = 2;
                        }
                        zwc = ww-parseInt(upper_right_X)-parseInt($(mqxians_Img).outerWidth());
                        zhc = hh-parseInt(lower_left_Y)-parseInt($(mqxians_Img).outerHeight());
                        mqxians_Img.style.width=zwc+"px";
                        mqxians_Img.style.height=zhc+"px";
                    }*/
                    //第六种：只靠右下角，
                    /*if(parseInt(xx) == parseInt($(ud).offset().left) || parseInt(yy) == parseInt($(ud).offset().top)){
                        // 最大缩放比例限制
                        if (newScale > 2) {
                            newScale = 2;
                        }
                        zwc = ww-parseInt(upper_right_X)-parseInt($(mqxians_Img).outerWidth());
                        zhc = hh-parseInt(lower_left_Y)-parseInt($(mqxians_Img).outerHeight());
                        mqxians_Img.style.width=zwc+"px";
                        mqxians_Img.style.height=zhc+"px";
                    }*/
                }
            });
            document.addEventListener('touchend', function () {//手指离开屏幕后触发
                store.moveable = false;
                delete store.pageX2;
                delete store.pageY2;

            });
            document.addEventListener('touchcancel', function () {//执行都不进来
                store.moveable = false;
                delete store.pageX2;
                delete store.pageY2;
            });
        },
    });
});
$("#zhu").click(function () {
    var zx = ($("#mqxians .nav-imgs").offset().left);
    var zy = ($("#mqxians .nav-imgs").offset().top);
    var zwc = ($("#mqxians .nav-imgs").outerWidth());
    var zhc = ($("#mqxians .nav-imgs").outerHeight());
    var zwv = $("#mqxians").width();
    var zhv = $("#mqxians").height();
    var zxv = $("#mqxians").offset().left;
    var zyv = $("#mqxians").offset().top;
    alert("1X:::"+zx+"y:::"+zy+"W:::"+zwc+"H:::"+zhc);
    alert("2x:::"+zxv+"2y:::"+zyv+":::"+zwv+":::"+zhv);
});

/**
 * 信号源管理操作
 */
var add_id ="";
var add_ipc = "";
var del_id = "";
var del_ipc = "";
$(document).ready(function (){
    $(".div-tdone span").click(function (e) {
        $(".div-ul").slideDown();
        $(".div-ultoo").slideUp();
        e.stopPropagation();
    });
    $(".div-tdtoo span").click(function (e) {
        $(".div-ultoo").slideDown();
        $(".div-ul").slideUp();
        e.stopPropagation();
    });
    $("#cfpo-ent7").click(function (e) {
        $(".div-ul").slideUp();
        $(".div-ultoo").slideUp();
    });

    var input_ru=document.getElementById("input_ru");
    var input_chu=document.getElementById("input_chu");
    var num=1,sum=1,num2=1,sum2=1,rusum=1,id=1;
    $("#submit1").click(function (){
        if(input_ru.value==""||input_ru.value==0){
           alert("请输入输入卡数量");
        }else {
            var ru=(input_ru.value)*4;
            for (var i = 0; i < ru; i++) {
                var $tr = $('<tr id="'+(id++)+'"></tr>');
                var $li1 = "<td class='id'><input type=\"checkbox\" name=\"check\" />" + (num++) + "</td>";
                var $li2 = "<td class='ru_id'>" + (sum++) + "</td>";
                var $li3 = "<td class='type'>" + "zz" + "</td>";
                var $li4 = "<td class='miao_shu'>" + "aa-"+(rusum++) + "</td>";
                var $li5 = "<td class='die_jia'>" + "cc" + "</td>";
                var $li6 = "<td></td>";
                $tr.append($li1, $li2, $li3, $li4, $li5, $li6);
                $('#list').append($tr);
                }
            $("#zeng-kuang").toggle("clip");
            $("#cfpo-dis").hide();
        }
    });
    $("#submit2").click(function (){
        if (input_chu.value==0||input_chu.value==""){
        alert("请输出输出卡数量");
        } else {
            var chusum = 1;
            var chu = (input_chu.value) * 4;
            var chu4 = (input_chu.value) * 2;
            var checkIndex = $("#select_chu ").get(0).selectedIndex;
            if (checkIndex == 3) {
                for (var j = 0; j < chu4 ; j++) {
                    var $tr = $("<tr></tr>");
                    var $li1 = "<td class='ben'><input type=\"checkbox\" name=\"checktoo\" /></td>";
                    var $li2 = "<td class='ese'>" + (sum2++) + "</td>";
                    var $li3 = "<td class='ru'>" + "zz" + "</td>";
                    var $li4 = "<td class='ru_ru'>" + "aa" + "</td>";
                    var $li5 = "<td class='ru_ru_id'>" + "cc" + "</td>";
                    var $li6 = "<td class='men'>" + (chusum++) + "</td>";
                    var $li7 = "<td></td>";
                    $tr.append($li1, $li2, $li3, $li4, $li5, $li6, $li7);
                    $('#list-too').append($tr);
                }
                $("#too-zeng-kuang").toggle("clip");
                $("#cfpo-dis").hide();

            } else {
                for (var j = 0; j < chu; j++) {
                    var $tr = $("<tr></tr>");
                    var $li1 = "<td class='ben'><input type=\"checkbox\" name=\"checktoo\" /></td>";
                    var $li2 = "<td class='ese'>" + (sum2++) + "</td>";
                    var $li3 = "<td class='ru'>" + "zz" + "</td>";
                    var $li4 = "<td class='ru_ru'>" + "aa" + "</td>";
                    var $li5 = "<td class='ru_ru_id'>" + "cc" + "</td>";
                    var $li6 = "<td class='men'>" + (chusum++) + "</td>";
                    var $li7 = "<td></td>";
                    $tr.append($li1, $li2, $li3, $li4, $li5, $li6, $li7);
                    $('#list-too').append($tr);
                }
                $("#too-zeng-kuang").toggle("clip");
                $("#cfpo-dis").hide();
            }
        }
    });
    $("#update").click(function () {
        var id = $('#hidId').val();
        var tds = $('#' + id + '>td');
        tds.eq(1).text($('#txtId').val());
        tds.eq(2).text($('#txtCountry').val());
        tds.eq(3).text($('#txtCapital').val());
        $('#' + id).attr('id', $('#txtId').val());
        $("#gai-kuang").toggle("clip");
    });

    //序号处理
    function sortNumberToo(){
        $('#list-too>tr').each(function (index, obj) {
            $(obj).find(".signal_number").html(index + 1);
        });
    }
    $("#signal_delete").click(function () {
        del_id = $("#list-too :checked").parents("tr").find(" td").eq(2).text();
        del_ipc = $("#list-too :checked").parents("tr").find(" td").eq(3).text();
        $("#list-too :checked").parents("tr").remove();
        sortNumberToo();
        setDelIpc();
        $('#signal_delete').css("background", "#c9cc19");
        $('#signal_delete').css("border","none");
        setTimeout(function () {
            $("#signal_delete").css("background", "transparent");
            $("#signal_delete").css("border","1px #fff solid");
        }, 200);
    });
    //序号处理
    function sortNumber(){
        $('#bod-table1 #list>tr').each(function(index, obj){
            $(obj).find(".number").html(index+1);
        });
    }
    //导入信号源
    $("#channel").click(function () {
        var bool = $("#list :checked").is(':checked');
        if($("tbody#list tr:visible").length==0){
            alert("请搜索信号源");
        }else if (bool==false){
            alert("请选择信号源");
        }else {
            var suu=1;
            var input=document.getElementById("list").getElementsByTagName("input");
            for(var i=0;i<input.length;i++){
                if(input[i].type=="checkbox"){
                    if(input[i].checked&&input[i].name=="check"){
                        var checkedRow=input[i];
                        var tr = checkedRow.parentNode.parentNode;
                        var tds = tr.cells;
                        //循环列
                        for(var j = 2;j < tds.length;j++){
                            var trid = $("<tr></tr>");
                            var $li1 = "<td class='signal_choose'><input type=\"checkbox\" name=\"checktoo\" value='2' /></td>";
                            var $li3 = "<td class='signal_id'>" + tds[2].innerHTML + "</td>";
                            var $li4 = "<td class='signal_ipc'>" + tds[3].innerHTML + "</td>";
                            var $li5 = "<td class='signal_user_name'>name</td>";
                            var $li6 = "<td class='signal_name'></td>";
                            var $li7 = "<td><input class='signal_alter' type='button' value='修改'/></td>";
                        }
                        trid.append($li1, "<td class='signal_number'>" + (suu++) + "</td>", $li3, $li4, $li5, $li6, $li7);
                        $("#list-too").append(trid);
                        add_id = tds[2].innerHTML;
                        add_ipc = tds[3].innerHTML;
                        setAddIpc();
                    }
                }
            }
            $("#list :checked").parents("tr").remove();
            sortNumber();
            $("#list-too tr").find(" input").attr("name", "checktoo");

            $('#channel').css("background", "#c9cc19");
            $('#channel').css("border","none");
            setTimeout(function () {
                $("#channel").css("background", "transparent");
                $("#channel").css("border","1px #fff solid");
            }, 200);
        }
    });
    //导出信号源
    $("#export").click(function () {
        var bool = $("#list-too :checked").is(':checked');
        if ($("tbody#list-too tr:visible").length == 0) {
            alert("请搜索信号源")
        } else if (bool == false) {
            alert("请选择信号源")
        } else {
            var suu = 1;
            var input = document.getElementById("list-too").getElementsByTagName("input");
            for (var i = 0; i < input.length; i++) {
                if (input[i].type == "checkbox") {
                    if (input[i].checked && input[i].name == "checktoo") {
                        var checkedRow = input[i];
                        var tr = checkedRow.parentNode.parentNode;
                        var tds = tr.cells;
                        //循环列
                        for (var j = 2; j < tds.length; j++) {
                            var $tr = $('<tr id="' + (id++) + '"></tr>');
                            var $li1 = "<td class='choose'><input type=\"checkbox\" name=\"check\" /></td>";
                            var $li3 = "<td class='input_id'>" + tds[2].innerHTML + "</td>";//输入卡ID
                            var $li4 = "<td class='input_ipc'>" + tds[3].innerHTML + "</td>";//输入卡IPC
                            var $li5 = "<td></td>";
                            var $li6 = "<td></td>";
                        }
                        $tr.append($li1, "<td class='number'>" + (suu++) + "</td>", $li3, $li4, $li5, $li6);
                        $("#list").append($tr);
                        add_id = tds[2].innerHTML;
                        add_ipc = tds[3].innerHTML;
                        //getAddIpc();
                        alert(add_id + "::" + add_ipc);
                    }
                }
            }
            $("#list-too :checked").parents("tr").remove();
            sortNumber();
            $("#list tr").find(" input").attr("name", "checktoo");

            $('#channel').css("background", "#c9cc19");
            $('#channel').css("border", "none");
            setTimeout(function () {
                $("#channel").css("background", "transparent");
                $("#channel").css("border", "1px #fff solid");
            }, 200);
        }
    });
    $("#search").click(function () {
        getSearchIpc();
        $('#search').css("background", "#c9cc19");
        $('#search').css("border","none");
        setTimeout(function () {
            $("#search").css("background", "transparent");
            $("#search").css("border","1px #fff solid");
        }, 200);
    });
    $("#export").click(function () {
        $('#export').css("background", "#c9cc19");
        $('#export').css("border","none");
        setTimeout(function () {
            $("#export").css("background", "transparent");
            $("#export").css("border","1px #fff solid");
        }, 200);
    });
});
function selectAll(selectStatus){
    if(selectStatus){
        $("input[name='check']").each(function(i,n){
            n.checked = true;
        });
    }else{
        $("input[name='check']").each(function(i,n){
            n.checked = false;
        });
    }
}
function selectAllToo(selectStatusToo){
    if(selectStatusToo){
        $("input[name='checktoo']").each(function(i,n){
            n.checked = true;
        });
    }else{
        $("input[name='checktoo']").each(function(i,n){
            n.checked = false;
        });
    }
}
$(document).ready(function () {
    $(".zeng").click(function () {
        $("#zeng-kuang").toggle("clip");
        $(".div-ul").slideUp();
        $("#cfpo-dis").show();
    });
    $(".too-zeng").click(function () {
        $("#too-zeng-kuang").toggle("clip");
        $(".div-ultoo").slideUp();
        $("#cfpo-dis").show();
    });
    $(".sang").click(function () {
        $("#list :checked").parents("tr").remove();
        $(".div-ul").slideUp();
    });

    $(".too-sang").click(function () {
        $("#list-too :checked").parents("tr").remove();
        $(".div-ultoo").slideUp();
    });
    $("#guan1").click(function () {
        $("#zeng-kuang").toggle("clip");
        $("#cfpo-dis").hide();
    });
    $("#guan2").click(function () {
        $("#gai-kuang").toggle("clip");
        $("#cfpo-dis").hide();
    });
    $("#guan3").click(function () {
        $("#too-zeng-kuang").toggle("clip");
        $("#cfpo-dis").hide();
    });
});
function re1_click() {
    $("#zeng-kuang").toggle("clip");
    $("#cfpo-dis").hide();
}
function re2_click() {
    $("#gai-kuang").toggle("clip");
    $("#cfpo-dis").hide();
}
function re3_click() {
    $("#too-zeng-kuang").toggle("clip");
    $("#cfpo-dis").hide();
}

/**
 * 信号源拖动操作，双击缩放，长按拖拽/点击删除，单指拖拽缩放
 * @type {string}
 */


/**
 * 球机控制操作
 */
//var zoo="";// zoom+表示放大
//var oom="";// zoom-表示缩小
//var x_="";//x+表示往右；x-表示往左
//var y_="";//y+表示往上；y-表示往下
$(document).ready(function () {
    $(".san-shang").click(function () {
        //y_="y+";
        //setPtzDirection();
        $(".triangle-top").css("background-image","url('../image/上2.png')");
        setTimeout(function(){
            $(".triangle-top").css("background-image","url('../image/上.png')");
        },300);
        console.log("上移");
    });
    $(".san-zuo").click(function () {
        //x_="x-";
        //setPtzDirection();
        $(".triangle-left").css("background-image","url('../image/左2.png')");
        setTimeout(function(){
            $(".triangle-left").css("background-image","url('../image/左.png')");
        },300);
        console.log("左移");
    });
    $(".san-xia").click(function () {
        //y_="y-";
        //setPtzDirection();
        $(".triangle-down").css("background-image","url('../image/下2.png')");
        setTimeout(function(){
            $(".triangle-down").css("background-image","url('../image/下.png')");
        },300);
        console.log("下移");
    });
    $(".san-you").click(function () {
        //x_="x+";
        //setPtzDirection();
        $(".triangle-right").css("background-image","url('../image/右2.png')");
        setTimeout(function(){
            $(".triangle-right").css("background-image","url('../image/右.png')");
        },300);
        console.log("右移");
    });

    $("#div-fang").click(function () {
        //zoo="zoom+";
        //setPtzZoom();
        $(".div-fang").css("background-image","url('../image/放大2.png')");
        setTimeout(function(){
            $(".div-fang").css("background-image","url('../image/放大.png')");
        },300);
        console.log("放大");
    });
    $("#div-suo").click(function () {
        //oom="zoom-";
        //setPtzZoom();
        $(".div-suo").css("background-image","url('../image/缩小2.png')");
        setTimeout(function(){
            $(".div-suo").css("background-image","url('../image/缩小.png')");
        },300);
        console.log("缩小");
    });
});

//禁止整个页面所有的右击事件
document.oncontextmenu = function(e){
    e.preventDefault();
  return false;
};
/*$("#ent4").on('touchstart', function(e){
    e.preventDefault();
},false);*/
//阻止默认长按出现菜单
/*window.ontouchstart = function(e) {
    e.preventDefault();
};*/
var userAgent = navigator.userAgent;
if (userAgent.indexOf("Safari") > -1) {
    window.onload=function () {
        document.addEventListener('touchstart',function (event) {
            if(event.touches.length>1){
                event.preventDefault();
            }
        })
        var lastTouchEnd=0;
        document.addEventListener('touchend',function (event) {
            var now=(new Date()).getTime();
            if(now-lastTouchEnd<=300){
                event.preventDefault();
            }
            lastTouchEnd=now;
        },false)
    }
}
document.addEventListener('gesturestart', function(event) {
   event.preventDefault();
});
var jinzhi=0;
document.addEventListener("touchmove",function(e){
    if(jinzhi==0){
        //e.preventDefault();
        e.stopPropagation();
    }
    jinzhi=1;
},false);

//强制显示为横屏，ipad的上如果是竖屏显示横的页面子级样式有点走样，对手机端无效果
// 直接在最外层的div调用该函数即可
changeOrientation($('.body'));
function changeOrientation( $print ){
    var width = document.documentElement.clientWidth;
    var height =  document.documentElement.clientHeight;
    if( width < height ){
      $print.width(height);
      $print.height(width);
      $print.css('top',  (height-width)/2 );
      $print.css('left',  0-(height-width)/2 );
      $print.css('transform' , 'rotate(90deg)');
      $print.css('transform-origin' , '50% 50%');
    }
    var evt = "onorientationchange" in window ? "orientationchange" : "resize";
    window.addEventListener(evt, function() {
        console.log(evt);
        setTimeout( function(){
            var width = document.documentElement.clientWidth;
             var height =  document.documentElement.clientHeight;
             if( width > height ){
                $print.width(width);
                $print.height(height);
                $print.css('top',  0 );
                $print.css('left',  0 );
                $print.css('transform' , 'none');
                $print.css('transform-origin' , '50% 50%');
             }
             else{
                $print.width(height);
                $print.height(width);
                $print.css('top',  (height-width)/2 );
                $print.css('left',  0-(height-width)/2 );
                $print.css('transform' , 'rotate(90deg)');
                $print.css('transform-origin' , '50% 50%');
             }
        }  , 300 );
    }, false);
}


//移动端作判断,信号源向上向下滚动按钮
if(/Android|webOS|iPhone|iPod|BlackBerry|iPad/i.test(navigator.userAgent)){
    $(".swiper-container-wen").show();
    $("#tree-pc").hide();
    (function($){
        $.fn.extend({
            Scroll:function(opt,callback){
                if(!opt) var opt={};
                var _btnUp = $("#"+ opt.up);
                var _btnDown = $("#"+ opt.down);
                var _this=this.eq(0).find("div:first");
                var lineH=_this.find("div:first").height(),
                line=opt.line?parseInt(opt.line,10):parseInt(this.height()/lineH,10),
                speed=opt.speed?parseInt(opt.speed,10):900;
                if(line==0) line=1;
                var upHeight=0-line*lineH-10;
                var scrollUp=function(){
                    _btnUp.unbind("click",scrollUp);
                    _this.animate({
                        marginTop:upHeight
                    },speed,function(){
                        for(i=1;i<=line;i++){
                            _this.find("div:first").appendTo(_this);
                        }
                        _this.css({marginTop:0});
                        _btnUp.bind("click",scrollUp);
                    });
                    $('.hua-xia').css("background-image","url('../image/下2.png')");
                    setTimeout(function(){
                        $('.hua-xia').css("background-image","url('../image/下.png')");
                    },300);
                };
                var scrollDown=function(){
                    _btnDown.unbind("click",scrollDown);
                    for(i=1;i<=line;i++){
                        _this.find("div:last").show().prependTo(_this);
                    }
                    _this.css({marginTop:upHeight});
                    _this.animate({
                        marginTop:0
                    },speed,function(){
                        _btnDown.bind("click",scrollDown);
                    });
                    $('.hua-shang').css("background-image","url('../image/上2.png')");
                    setTimeout(function(){
                        $('.hua-shang').css("background-image","url('../image/上.png')");
                    },300);
                };
                _btnUp.css("cursor","pointer").click( scrollUp );
                _btnDown.css("cursor","pointer").click( scrollDown );
            }
        });
    })(jQuery);
    $(document).ready(function(){
        $(".tree-slio").Scroll({line:1,speed:600,timer:3000,up:"hua-xia",down:"hua-shang"});
    });
}else {
    $(".swiper-container-wen").hide();
    $("#tree-pc").show();
    (function($){
        $.fn.extend({
            Scroll:function(opt,callback){
                if(!opt) var opt={};
                var _btnUp = $("#"+ opt.up);
                var _btnDown = $("#"+ opt.down);
                var _this=this.eq(0).find("div:first");
                var lineH=_this.find("div:first").height(),
                line=opt.line?parseInt(opt.line,10):parseInt(this.height()/lineH,10),
                speed=opt.speed?parseInt(opt.speed,10):900;
                if(line==0) line=1;
                var upHeight=0-line*lineH-10;
                var scrollUp=function(){
                    _btnUp.unbind("click",scrollUp);
                    _this.animate({
                        marginTop:upHeight
                    },speed,function(){
                        for(i=1;i<=line;i++){
                            _this.find("div:first").appendTo(_this);
                        }
                        _this.css({marginTop:0});
                        _btnUp.bind("click",scrollUp);
                    });
                    $('.hua-xia').css("background-image","url('../image/下2.png')");
                    setTimeout(function(){
                        $('.hua-xia').css("background-image","url('../image/下.png')");
                    },300);
                };
                var scrollDown=function(){
                    _btnDown.unbind("click",scrollDown);
                    for(i=1;i<=line;i++){
                        _this.find("div:last").show().prependTo(_this);
                    }
                    _this.css({marginTop:upHeight});
                    _this.animate({
                        marginTop:0
                    },speed,function(){
                        _btnDown.bind("click",scrollDown);
                    });
                    $('.hua-shang').css("background-image","url('../image/上2.png')");
                    setTimeout(function(){
                        $('.hua-shang').css("background-image","url('../image/上.png')");
                    },300);
                };
                _btnUp.css("cursor","pointer").click( scrollUp );
                _btnDown.css("cursor","pointer").click( scrollDown );
            }
        });
    })(jQuery);
    $(document).ready(function(){
        $(".tree-slio-pc").Scroll({line:1,speed:600,timer:3000,up:"hua-xia",down:"hua-shang"});
    });
}
$("#fu2").click(function () {
    $(this).css({"background":"#3fe6f7","color":"#fff"});
    var _this = this;
    setTimeout(function(){
        $(_this).css({"background":"transparent","color":"#00bcd4"});
    },200);
    setPipWidgetAllClose();
    $("#mqxians .tree-xin,#mqxians .nav-imgs").remove();
    playVideo();
    // if(fu_count++%2 == 0){
    //     $("#fu2 button").html("关闭回显");
    // }else {
    //     $("#fu2 button").html("开启回显");
    // }
});
