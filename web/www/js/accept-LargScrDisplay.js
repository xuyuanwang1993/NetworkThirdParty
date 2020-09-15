//传输幕墙指令
function setJointSet(){
    console.log(dev_row);
    console.log(dev_col);
    $.ajax({
        type: "post",
        url: "../cgi-bin/web_function.cgi",
        data: 'cmd : JointSet\r\nrow : '+dev_row+'\r\ncol : '+dev_col+'\r\n',
        async: true,
        success:function (data) {
        },
    });
}

//开窗漫游,开窗
var sum="";
function setNetBasicInfo(){
    var dev_sm=$('#mqxians .tree-xin,#mqxians .nav-imgs');
    sum=dev_sm.length;
    $(tree_xin).find('.tree-xin,.nav-imgs').attr('id',sum);
    var div1=$('td[id^="com"]').outerWidth();
    var div2=$('td[id^="com"]').outerHeight();
    var div3=$('td[id^="com"]').offset().left;
    var div4=$('td[id^="com"]').offset().top;
    var wth=1920,hht=1080,width=w,height=h;
    var v1=0,v2=0;
    var s1=1920,s3=1080;
    var c1=width,c5=height;
    var w1=0,w2=0;

    var id_tree_xin=$(tree_xin).attr('id');
    var dev=id_tree_xin.split('com')[1];
    $(tree_xin).find(' div').find(' input').attr('value',dev);
    console.log("src_dev : "+dev);
    var code='dev_'+dev+' : '+v1.toFixed(0)+':'+v2.toFixed(0)+':'+s1.toFixed(0)+':'+s3.toFixed(0)+':'+w1.toFixed(0)+':'+w2.toFixed(0)+':'+c1+':'+c5;
    var dev_digital ="";
    for (var i = 0; i < 9; i++) {
        if (i == parseInt(dev)) {
            dev_digital+=1;
        }else {
            dev_digital+=0;
        }
    }
    var digital="";
    for (var i = dev_digital.length - 1; i >= 0; i--) {
        digital += dev_digital[i];
    }
    var plan_mode = parseInt(digital, 2);
    console.log(plan_mode + ":::::" + digital);
    console.log(sum);
    console.log(kc_id);
    console.log(code);
    console.log("src_dev : " + dev);
    $.ajax({
        type:"post",
        url:'../cgi-bin/web_function.cgi',
        data:"cmd : PipWidgetOpen\r\nwin_id : "+sum+"\r\nipc_id : "+kc_id+"\r\nplan_mode : "+plan_mode+"\r\nstream_id : "+0+"\r\ndev_num : "+1+"\r\nsrc_dev : "+dev+"\r\n"+code+"\r\n",
        async:true,
        success:function (data){
            },
    });
}
//开窗移动，大小改变，显示最上层
var Upper_left_Y="",Upper_left_X="";
function setPipWidgetChange() {
    //窗口的位置信息
    var div1=$('td[id^="com"]').outerWidth()+1;//窗口W
    var div2=$('td[id^="com"]').outerHeight()+1;//窗口H
    var div3=$('td[id^="com"]').offset().left;//窗口X
    var div4=$('td[id^="com"]').offset().top;//窗口Y
    console.log("div1:::" + div1 + "::div2:::" + div2);
    console.log("div3:::" + div3 + ":div4:::" + div4);

    var width=pip_w,height=pip_h;

    var row = document.getElementById("input_row");//行
    var col = document.getElementById("input_col");//列
    var dev_row=inp_row;//行
    var dev_col=inp_col;//列

    //获取左（上下），右(上下)教的位置
    var lower_right_X=(pip_X-div3)+pip_W;//右下角X
    var lower_right_Y =(pip_Y-div4) + pip_H; //右下角Y
    var upper_right_X=(pip_X-div3)+pip_W;//右上角X
    var upper_right_Y=(pip_Y-div4)+0;//右上角Y
    var Lower_left_X=(pip_X-div3)+0;//左下角X
    var Lower_left_Y=(pip_Y-div4)+pip_H;//左下角Y
    var Upper_left_X=(pip_X-div3);//左上角X
    var Upper_left_Y=(pip_Y-div4);//左上角Y
    if(upper_right_Y<0){
        upper_right_Y=0;
    }if(Lower_left_X<0){
        Lower_left_X=0;
    }if(Upper_left_X<0){
        Upper_left_X=0;
    }if(Upper_left_Y<0){
        Upper_left_Y=0;
    }
    console.log("Upper_left_X左上角X:" + Upper_left_X.toFixed(0));
    console.log("Upper_left_Y左上角Y:" + Upper_left_Y.toFixed(0));
    console.log("lower_right_X右下角X:" + lower_right_X.toFixed(0));
    console.log("lower_right_Y右下角Y:" + lower_right_Y.toFixed(0));
    console.log("upper_right_X右上角X:" + upper_right_X.toFixed(0));
    console.log("upper_right_Y右上角Y:" + upper_right_Y.toFixed(0));
    console.log("Lower_left_X左下角X:" + Lower_left_X.toFixed(0));
    console.log("Lower_left_Y左下角Y:" + Lower_left_Y.toFixed(0));

    var wth=1920,hht=1080;

    var one_row=(Upper_left_X+0.0000023)/div1;//第一个行
    var one_col=(Upper_left_Y+0.0000023)/div2;//第一个列
    var two_row=(upper_right_X)/div1;//第二个行
    var two_col=(upper_right_Y+0.0000023)/div2;//第二个列
    var three_row=(Lower_left_X+0.0000023)/div1;//第三个行
    var three_col=(Lower_left_Y)/div2;//第三个列
    var four_row=(lower_right_X)/div1;//第四个行
    var four_col=(lower_right_Y)/div2;//第四个列

    var one=Math.ceil(one_col)*dev_col-(dev_col-Math.ceil(one_row));//第一个角占的屏幕ID
    var two=Math.ceil(two_col)*dev_col-(dev_col-Math.ceil(two_row));//第二个角占的屏幕ID
    var three=Math.ceil(three_col)*dev_col-(dev_col-Math.ceil(three_row));//第三个角占的屏幕ID
    var four=Math.ceil(four_col)*dev_col-(dev_col-Math.ceil(four_row));//第四个角占的屏幕ID

    console.log("第一个角：(" + Math.ceil(one_col) + "," + Math.ceil(one_row) + ")");
    console.log("第二个角：(" + Math.ceil(two_col) + "," + Math.ceil(two_row) + ")");
    console.log("第三个角：(" + Math.ceil(three_col) + "," + Math.ceil(three_row) + ")");
    console.log("第四个角：(" + Math.ceil(four_col) + "," + Math.ceil(four_row) + ")");

    var ww1=(Math.ceil(one_row)*div1)-Upper_left_X.toFixed(0);//第一个角占的屏宽
    var w1=ww1*(wth/div1);//第一个角占的实际屏宽
    var hh1=(Math.ceil(one_col)*div2)-Upper_left_Y.toFixed(0);//第一个角占的屏高
    var h1=hh1*(hht/div2);//第一个角占的实际屏高
    var xx1=div1-((Math.ceil(one_row)*div1)-Upper_left_X.toFixed(0));//第一个角占的X坐标
    var x1=xx1*(wth/div1);//第一个角占的实际X坐标
    var yy1=div2-((Math.ceil(one_col)*div2)-Upper_left_Y.toFixed(0));//第一个角占的Y坐标
    var y1=yy1*(hht/div2);//第一个角占的实际Y坐标
    var W1=(ww1/pip_W)*width;//按1920来分值的宽
    var H1=(hh1/pip_H)*height;//按1080来分值的高
    var X1=0;var Y1=0;//按1920，1080来分值的X,Y坐*标

    var ww2=div1-((Math.ceil(two_row)*div1)-upper_right_X.toFixed(0));//第二个角占的屏宽
    var w2=ww2*(wth/div1);//第二个角占的实际屏宽
    var h2=h1;//第二个角占的实际屏高
    var x2=0;//第二个角占的实际X坐标
    var y2=y1;//第二个角占的实际Y坐标
    var W2=(ww2/pip_W)*width;//按1920来分值的宽
    var H2=H1;//按1080来分值的高
    var XX2=(Math.ceil(two_row))*div1-Upper_left_X.toFixed(0)-div1;//X坐标
    var X2=(XX2/pip_W)*width;//按1920来分值的X坐标
    var Y2=0;//按1080来分值的Y坐标

    var w3=w1;//第三个角占的实际屏宽
    var hh3=div2-((Math.ceil(three_col)*div2)-Lower_left_Y.toFixed(0));//第三个角占的屏高
    var h3=hh3*(hht/div2);//第三个角占的实际屏高
    var x3=x1;//第三个角占的实际X坐标
    var y3=0;//第三个角占的实际Y坐标
    var W3=W1;//按1920来分值的宽
    var H3=(hh3/pip_H)*height;//按1080来分值的高
    var X3=0;//按1080来分值的Y坐标
    var YY3=(Math.ceil(three_col)*div2)-Upper_left_Y.toFixed(0)-div2;//Y坐标
    var Y3=(YY3/pip_H)*height;//按1080来分值的Y坐标

    var w4=w2;//第四个角占的实际屏宽
    var h4=h3;//第四个角占的实际屏高
    var x4=0;//第四个角占的实际X坐标
    var y4=0;//第四个角占的实际Y坐标
    var W4=W2;//按1920来分值的宽
    var H4=H3;//按1080来分值的高
    var X4=X2;//按1920来分值的X坐标
    var Y4=Y3;//按1080来分值的Y坐标

    var dev_id="";
    var dev_c1=(Math.ceil(one_col)*Math.ceil(four_row)-(Math.ceil(four_row)-Math.ceil(one_row)))-1;
    var dev_c2=(Math.ceil(two_col)*Math.ceil(four_row)-(Math.ceil(four_row)-Math.ceil(two_row)))-1;
    var dev_c3=(Math.ceil(three_col)*Math.ceil(four_row)-(Math.ceil(four_row)-Math.ceil(three_row)))-1;
    var dev_c4=(Math.ceil(four_col)*Math.ceil(four_row)-(Math.ceil(four_row)-Math.ceil(four_row)))-1;
    if (W1>width) {
        W1=width;
    }
    if (H1>height){
        H1=height;
    }
    if (W2>width) {
        W2=width;
    }
    if (H2>height){
        H2=height;
    }
    if (W3>width) {
        W3=width;
    }
    if (H3>height){
        H3=height;
    }
    if (W4>width) {
        W4=width;
    }
    if (H4>height){
        H4=height;
    }
    var m=div1-((Math.ceil(one_row)*div1)-Upper_left_X.toFixed(0));//左上角X
    var n=div2-((Math.ceil(one_col)*div2)-Upper_left_Y.toFixed(0));//左上角Y
    if((pip_W<div1)&&(m<div1)&&(Math.ceil(two_row)==Math.ceil(one_row))){
        w1=pip_W*(wth/div1);
        w3=pip_W*(wth/div1);

    }
    if((pip_H<div2)&&(n<div2)&&(Math.ceil(one_col)==Math.ceil(three_col))){
        h1=pip_H*(hht/div2);
        h2=pip_H*(hht/div2);
    }
    if(td_cont==""||td_cont==null||td_cont==undefined||td_cont==NaN){
        td_cont=0;
    }
    if (w1.toFixed(0) == 1904) {
        w1 = 1920;
    }
    if (w2.toFixed(0) == 1904) {
        w2 = 1920;
    }
    if (w3.toFixed(0) == 1904) {
        w3 = 1920;
    }
    if (w4.toFixed(0) == 1904) {
        w4 = 1920;
    }
    if (h1.toFixed(0) == 1056) {
        h1 = 1080;
    }
    if (h2.toFixed(0) == 1056) {
        h2 = 1080;
    }
    if (h3.toFixed(0) == 1056) {
        h3 = 1080;
    }
    if (h4.toFixed(0) == 1056) {
        h4 = 1080;
    }
    dev_id+="dev_"+(one-1+td_cont)+" : "+x1.toFixed(0)+":"+y1.toFixed(0)+":"+w1.toFixed(0)+":"+h1.toFixed(0)+":"+X1.toFixed(0)+":"+Y1.toFixed(0)+":"+Math.round(W1)+":"+Math.round(H1)+"\r\n";
    dev_id+="dev_"+(two-1+td_cont)+" : "+x2.toFixed(0)+":"+y2.toFixed(0)+":"+w2.toFixed(0)+":"+h2.toFixed(0)+":"+X2.toFixed(0)+":"+Y2.toFixed(0)+":"+Math.round(W2)+":"+Math.round(H2)+"\r\n";
    dev_id+="dev_"+(three-1+td_cont)+" : "+x3.toFixed(0)+":"+y3.toFixed(0)+":"+w3.toFixed(0)+":"+h3.toFixed(0)+":"+X3.toFixed(0)+":"+Y3.toFixed(0)+":"+Math.round(W3)+":"+Math.round(H3)+"\r\n";
    dev_id+="dev_"+(four-1+td_cont)+" : "+x4.toFixed(0)+":"+y4.toFixed(0)+":"+w4.toFixed(0)+":"+h4.toFixed(0)+":"+X4.toFixed(0)+":"+Y4.toFixed(0)+":"+Math.round(W4)+":"+Math.round(H4)+"\r\n";

    console.log("dev_" + (dev_c1 + td_cont) + "第一个角占的屏幕ID：" + one + " :x：" + x1.toFixed(0) + " :y：" + y1.toFixed(0) + " :宽：" + w1.toFixed(0) + " :高：" + h1.toFixed(0) + " :X1：" + X1.toFixed(0) + " :Y1：" + Y1.toFixed(0) + " :W1：" + Math.round(W1) + " :H1：" + Math.round(H1));
    console.log("dev_" + (dev_c2 + td_cont) + "第二个角占的屏幕ID：" + two + " :x2：" + x2.toFixed(0) + " :y2：" + y2.toFixed(0) + " :宽：" + w2.toFixed(0) + " :高：" + h2.toFixed(0) + " :X2：" + X2.toFixed(0) + " :Y2：" + Y2.toFixed(0) + " :W2：" + Math.round(W2) + " :H2：" + Math.round(H2));
    console.log("dev_" + (dev_c3 + td_cont) + "第三个角占的屏幕ID：" + three + " :x3：" + x3.toFixed(0) + " :y3：" + y3.toFixed(0) + " :宽：" + w3.toFixed(0) + " :高：" + h3.toFixed(0) + " :X3：" + X3.toFixed(0) + " :Y3：" + Y3.toFixed(0) + " :W3：" + Math.round(W3) + " :H3：" + Math.round(H3));
    console.log("dev_" + (dev_c4 + td_cont) + "第四个角占的屏幕ID：" + four + " :x4：" + x4.toFixed(0) + " :y4：" + y4.toFixed(0) + " :宽：" + w4.toFixed(0) + " :高：" + h4.toFixed(0) + " :X4：" + X4.toFixed(0) + " :Y4：" + Y4.toFixed(0) + " :W4：" + Math.round(W4) + " :H4：" + Math.round(H4));

    var one_two_row,one_two_col,three_four_row,three_four_col,dev_c;
    if ((two-one)>1){
        console.log("你正在横着跨屏");
        //循环算出中间跨的屏
        for (var i=Math.ceil(one_row)+1;i<Math.ceil(two_row);i++){//第一行几列
            one_two_row=one_col;
            one_two_col=i;
            var id_onetwo=(Math.ceil(one_two_row)*dev_col-(dev_col-one_two_col))-1;
            dev_c=Math.ceil(one_two_row)*Math.ceil(four_row) - (Math.ceil(four_row) - one_two_col)-1;
            var W=div1*(wth/div1);//宽
            var H=h1;//高
            var X=0;Y=y1;
            var w=(div1/pip_W)*width;//按1920来分值的宽
            var h=H1;//按1080来分值的高
            var xx=(one_two_col*div1)-Upper_left_X.toFixed(0)-div1;
            var x=(xx/pip_W)*width;//按1920来分值X坐标
            var y=0;//按1080来分值Y坐标
            if(H1>hht){
                H1=hht;
            }
            if(pip_H<div2&&Upper_left_Y<div2){
                H=pip_H*(hht/div2);
            }
            dev_id+="dev_"+(id_onetwo+td_cont)+" : "+X.toFixed(0)+":"+Y.toFixed(0)+":"+W.toFixed(0)+":"+H.toFixed(0)+":"+x.toFixed(0)+":"+y.toFixed(0)+":"+w.toFixed(0)+":"+h.toFixed(0)+"\r\n";
            console.log("dev_"+id_onetwo+" :行："+Math.ceil(one_two_row)+" :列："+one_two_col+" :屏幕ID："+id_onetwo+" :X："+X.toFixed(0)+" :Y："+Y.toFixed(0)+" :宽："+W.toFixed(0)+" :高："+H.toFixed(0)+" :x"+x.toFixed(0)+" :y："+y.toFixed(0)+" :w："+w.toFixed(0)+" :h："+h.toFixed(0)+"\n");
        }
        for (var k=Math.ceil(three_row)+1;k<Math.ceil(four_row);k++){//第三行几列
            three_four_row=three_col;
            three_four_col=k;
            var id_threefour=(Math.ceil(three_four_row)*dev_col-(dev_col-three_four_col))-1;
            dev_c=Math.ceil(three_four_row)*Math.ceil(four_row) - (Math.ceil(four_row) - three_four_col)-1;
            var W=div1*(wth/div1);//宽
            var H=h3;//高
            var X=0;var Y=0;
            var w=(div1/pip_W)*width;//按1920来分值的宽
            var h=H3;//按1080来分值的高
            var xx=(three_four_col*div1)-Upper_left_X.toFixed(0)-div1;
            var x=(xx/pip_W)*width;//按1920来分值X坐标
            var yy=(Math.ceil(three_four_row)*div2)-Upper_left_Y.toFixed(0)-div2;
            var y=(yy/pip_H)*height;//按1080来分值Y坐标
            if(H1>hht){
                H1=hht;
            }
            if(pip_H<div2&&Upper_left_Y<div2){
                H=pip_H*(hht/div2);
            }
            dev_id+="dev_"+(id_threefour+td_cont)+" : "+X.toFixed(0)+":"+Y.toFixed(0)+":"+W.toFixed(0)+":"+H.toFixed(0)+":"+x.toFixed(0)+":"+y.toFixed(0)+":"+w.toFixed(0)+":"+h.toFixed(0)+"\r\n";
            console.log("dev_" + id_threefour + " :行：" + Math.ceil(three_four_row) + " :列：" + three_four_col + " :屏幕ID：" + id_threefour + " :X：" + X.toFixed(0) + " :Y：" + Y.toFixed(0) + " :宽：" + W.toFixed(0) + " :高：" + H.toFixed(0) + " :x" + x.toFixed(0) + " :y：" + y.toFixed(0) + " :w：" + w.toFixed(0) + " :h：" + h.toFixed(0) + "\n");
        }
    }else {
        console.log("无横跨屏");
    }
    var one_three_row,one_three_col,two_four_row,two_four_col;
    if ((three-one)>1){
        console.log("你正在竖着跨屏");
        for (var j=Math.ceil(one_col)+1;j<Math.ceil(three_col);j++){//第一列几行
            one_three_col=one_row;
            one_three_row=j;
            var id_onethree=(one_three_row*dev_col-(dev_col-Math.ceil(one_three_col)))-1;
            dev_c=one_three_row*Math.ceil(four_row) - (Math.ceil(four_row) - Math.ceil(one_three_col))-1;
            var W=w1;//宽
            var H=div2*(hht/div2);//高
            var X=x1;var Y=0;
            var w=W1;//按1920来分值的宽
            var h=(div2/pip_H)*height;//按1080来分值的高
            var x=0;//按1920来分值X坐标
            var yy=(one_three_row*div2)-Upper_left_Y.toFixed(0)-div2;
            var y=(yy/pip_H)*height;//按1080来分值Y坐标
            if (W1>wth) {
                W1=1920;
            }
            if(pip_W<div1&&Upper_left_X<div1){
                W=pip_W*(wth/div1);
            }
            dev_id+="dev_"+(id_onethree+td_cont)+" : "+X.toFixed(0)+":"+Y.toFixed(0)+":"+W.toFixed(0)+":"+H.toFixed(0)+":"+x.toFixed(0)+":"+y.toFixed(0)+":"+w.toFixed(0)+":"+h.toFixed(0)+"\r\n";
            console.log("dev_" + id_onethree + " :行：" + one_three_row + " :列：" + Math.ceil(one_three_col) + " :屏幕ID：" + id_onethree + " :X：" + X.toFixed(0) + " :Y：" + Y.toFixed(0) + " :宽：" + W.toFixed(0) + " :高：" + H.toFixed(0) + " :x" + x.toFixed(0) + " :y：" + y.toFixed(0) + " :w：" + w.toFixed(0) + " :h：" + h.toFixed(0) + "\n");
        }
        for (var m=Math.ceil(two_col)+1;m<Math.ceil(four_col);m++){//第三列几行
            two_four_col=two_row;
            two_four_row=m;
            var id_twofour=(two_four_row*dev_col-(dev_col-Math.ceil(two_four_col)))-1;
            dev_c=two_four_row*Math.ceil(four_row) - (Math.ceil(four_row) - Math.ceil(two_four_col))-1;
            var W=w2;//宽
            var H=div2*(hht/div2);//高
            var X=0;var Y=0;
            var w=W2;//按1920来分值的宽
            var h=(div2/pip_H)*height;//按1080来分值的高
            var xx=(Math.ceil(two_four_col)*div1)-Upper_left_X.toFixed(0)-div1;
            var x=(xx/pip_W)*width;//按1920来分值X坐标
            var yy=(two_four_row*div2)-Upper_left_Y.toFixed(0)-div2;
            var y=(yy/pip_H)*height;//按1080来分值Y坐标
            if (W1>wth) {
                W1=1920;
            }
            if(pip_W<div1&&Upper_left_X<div1){
                W=pip_W*(wth/div1);
            }
            dev_id+="dev_"+(id_twofour+td_cont)+" : "+X.toFixed(0)+":"+Y.toFixed(0)+":"+W.toFixed(0)+":"+H.toFixed(0)+":"+x.toFixed(0)+":"+y.toFixed(0)+":"+w.toFixed(0)+":"+h.toFixed(0)+"\r\n";
            console.log("dev_" + id_twofour + " :行：" + two_four_row + " :列：" + Math.ceil(two_four_col) + " :屏幕ID：" + id_twofour + " :X：" + X.toFixed(0) + " :Y：" + Y.toFixed(0) + " :宽：" + W.toFixed(0) + " :高：" + H.toFixed(0) + " :x" + x.toFixed(0) + " :y：" + y.toFixed(0) + " :w：" + w.toFixed(0) + " :h：" + h.toFixed(0) + "\n");
        }
    }else {
        console.log("无竖跨屏");
    }
    if ((three-one)>1&&(two-one)>1) {
        console.log("预判正中心的屏幕");
        var onth_tow_row="";
        var onth_tow_col="";
        for(var q=Math.ceil(one_two_row)+1;q<Math.ceil(three_four_row);q++) {
            for (var t = Math.ceil(one_three_col)+1; t < Math.ceil(two_four_col); t++) {//第几行几列
                onth_tow_row = q;
                onth_tow_col = t;
                var id_onthtow = (onth_tow_row*dev_col - (dev_col - onth_tow_col))-1;
                dev_c=onth_tow_row*Math.ceil(four_row) - (Math.ceil(four_row) - onth_tow_col)-1;
                var W=wth;var H=hht;var X=0;var Y=0;
                var w=(div1/pip_W)*width;//按1920来分值的宽
                var h=(div2/pip_H)*height;//按1080来分值的高
                var xx=(onth_tow_col*div1)-Upper_left_X.toFixed(0)-div1;
                var x=(xx/pip_W)*width;//按1920来分值X坐标
                var yy=(onth_tow_row*div2)-Upper_left_Y.toFixed(0)-div2;
                var y=(yy/pip_H)*height;//按1080来分值Y坐标
                dev_id+="dev_"+(id_onthtow+td_cont)+" : "+X+":"+Y+":"+W+":"+H+":"+x.toFixed(0)+":"+y.toFixed(0)+":"+w.toFixed(0)+":"+h.toFixed(0)+"\r\n";
                console.log("dev_" + id_onthtow + " :行：" + onth_tow_row + " :列：" + Math.ceil(onth_tow_col) + " :屏幕ID：" + id_onthtow + " :X：" + X.toFixed(0) + " :Y：" + Y.toFixed(0) + " :宽：" + W.toFixed(0) + " :高：" + H.toFixed(0) + " :x" + x.toFixed(0) + " :y：" + y.toFixed(0) + " :w：" + w.toFixed(0) + " :h：" + h.toFixed(0) + "\n");
            }
        }
    }
    console.log(dev_id);
    var cha=dev_id.split("\r\n");
    var g="";
    cha=function(o){
        var array=new Array();
        for(g in o){
            if(o[g].indexOf('-')==-1){
                array.push(o[g]);
            }
        }
        return array;
    }(cha);
    console.log(cha);

    function noRepeat(cha) {
        var newArr = [];
        var indexArr = [];
        var setArr = [];
        for (const i in cha) {
        var moment = cha[i].split(":")[0];
        if (newArr.indexOf(moment) == -1) {
            newArr.push(moment);
                indexArr.push(i);
            }
        }
        for (const j in indexArr) {
            setArr.push(cha[j])
        }
        return setArr;
    }
    var result = noRepeat(cha);
    console.log(result);

    var de="";
    for (var z=0;z<result.length;z++){
        de+=result[z]+'\r\n';
    }

    var dev_pos = result.filter(s => $.trim(s).length > 0);
    var de_posz = "";
    var dev_num = "";
    for (var i = 0; i < dev_pos.length; i++) {
        var con_s = JSON.stringify(dev_pos[i]);
        var con_ss = con_s.replace(/\"/g, "");
        console.log(con_ss);
        var de_str = con_ss.match(/\_(\d+)/);
        console.log(de_str[1]);
        de_posz += de_str[1] + ",";
    }
    console.log(de_posz);
    var plan_mode = de_posz.split(",");
    plan_mode.sort((a, b) => a - b);
    plan_mode = plan_mode.filter(s => $.trim(s).length > 0);
    var plan_modes = plan_mode.map(Number);
    console.log(plan_modes);
    for (var v = 0; v < (dev_row * dev_col); v++) {
        if (plan_modes.indexOf(v) != -1) {
            dev_num += 1;
        } else {
            dev_num += 0;
        }
    }
    var digital="";
    for (var i = dev_num.length - 1; i >= 0; i--) {
        digital += dev_num[i];
    }
    var dev_nums = parseInt(digital, 2);
    console.log(dev_nums + ":::" + digital);
    console.log("当前占的窗口win_id : " + pip_tree_xin);
    console.log("当前开窗的ipc_id : " + pip_kc_id);
    console.log("stream_id : " + 0);
    console.log("占了几个屏dev_num : " + (cha.length - 1));
    console.log("src_dev : " + src_val);
    console.log(de);
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : PipWidgetChange\r\nwin_id : '+pip_tree_xin+'\r\nipc_id : '+pip_kc_id+"\r\nplan_mode : "+dev_nums+'\r\nstream_id : '+0+'\r\ndev_num : '+(cha.length-1)+'\r\nsrc_dev : '+src_val+'\r\n'+de+'\r\n',
        async:false,
        success:function (data) {
        },
async:true
    });
}
//获取保存了多少个预案
/*function setSceneInfo() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SceneInfo\r\n',
        async:false,
        success:function (data) {
                console.log(data);
                if($.trim(data)=="error"||$.trim(data)==""){
                    alert("获取多少个预案信息失败！"+data);
                }else {
                    var array = data.split("\r\n");
                    var scene_id="",name="",scene="",q=0,w=0,e=0,t=0;
                    for (var i = 0; i < array.length; i++) {
                        if ($.trim(array[i]).indexOf("scene_id") >= 0) {//场景id号
                            scene_id=$.trim(array[i].split(" : ")[1]);
                        } else if ($.trim(array[i]).indexOf("name") >= 0) {//场景名称
                            name=$.trim(array[i].split(" : ")[1]);
                            scene+='<li id="'+scene_id+'">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="co_name">'+name+'</span>' +
                                   '<span class="checkboxWon">' +
                                   '<input type="checkbox" id="'+(c+1)+'-'+(k+1)+'" class="reguar-checkbox sen test_0" autocomplete="off"/>' +
                                   '<label for="'+(b+1)+'-'+(r+1)+'"></label>' +
                                   '</span>' +
                                   '<span class="icon-blo reDel">' +
                                   '<div class="icon-noo"></div>' +
                                   '</span>'+
                                   '<span class="diaoyong">调用</span>' +
                                   '</li>';
                        }
                        $('#yuan-ul ul').html(scene);
                    }
                }
        },
    });
}*/
//调用预案
function setSceneCall() {
    $.ajax({
       type:'post',
       url:'../cgi-bin/web_function.cgi',
       data:'cmd : SceneCall\r\nscene_id : '+scene_id+'\r\n',
       async:true,
       success:function (data) {
        }
    });
}
//预案保存
function setSceneRecord() {
    var Yuan="";
    var num_r=$("#yuan-ul ul li").length;
    var nums=num_r+1;
    Yuan+='<li id="'+nums+'">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span class="co_name">'+scene_name+'</span>' +
            '<span class="checkboxWon">' +
                '<input type="checkbox" id="zc'+c+'-'+k+'" class="reguar-checkbox sen test_0" autocomplete="off"/>' +
                    '<label for="zc'+b+'-'+r+'"></label>' +
            '</span>' +
            '<span class="icon-blo reDel">' +
                '<div class="icon-noo"></div>' +
            '</span>'+
            '<span class="diaoyong">调用</span>' +
        '</li>';
   $('#yuan-ul ul').append(Yuan);
   $.ajax({
       type:'post',
       url:'../cgi-bin/web_function.cgi',
       data:'cmd : SceneRecord\r\nscene_id : '+nums+'\r\nname : '+scene_name+'\r\n',
       async:true,
       success:function (data) {
       }
   });
}
//获取开窗信息
function setPipWidgetInfo() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : PipWidgetInfo\r\n',
        async:true,
        success:function (data) {
            if($.trim(data)=="error"||$.trim(data)==""){
                alert("获取开窗信息失败！");
            }else {
                var array1 = data;
                console.log(array1);
                var array = data.split("\r\n");
                var win_id="",ipc_id="",arc="",_w=0,_h=0;

                for (var i = 0; i < array.length; i++) {
                    if ($.trim(array[i]).indexOf("win_id") >= 0) {//场景id号
                        win_id=$.trim(array[i].split(" : ")[1]);
                        console.log("当前窗口的ID号：" + win_id);
                    } else if ($.trim(array[i]).indexOf("ipc_id") >= 0) {//场景名称
                        ipc_id=$.trim(array[i].split(" : ")[1]);
                        console.log("设备ipc_id号:" + ipc_id);
                    }
                }
                var row = document.getElementById("input_row");//行
                var col = document.getElementById("input_col");//列
                var dev_row=row.value;//行
                var dev_col=col.value;//列

                var cont_e=0,con_s="";
                //窗口的位置信息
                var div1=$('#com0').outerWidth()+1;//窗口W
                var div2=$('#com0').outerHeight()+1;//窗口H
                var artr=array1.split("\r\nstream_id : 0\r\n").map(e=>e.split("\r\n").map(ee=>{
                    var s=ee.split(" : ");
                    if(s.length==2)return {[s[0]]:s[1]};
                }).filter(e=>e)).filter(e=>e.length);
                for (var i=1;i<artr.length;i++){
                    arc=artr[i][0];
                    console.log(arc);
                    con_s= JSON.stringify(arc);
                    var con_ss=con_s.replace(/\"/g,"");
                    console.log(con_ss);

                    var arc2=artr[i];
                    var con_s2= JSON.stringify(arc2);
                    var con_ss2=con_s2.replace(/\"/g,"");
                    var index = con_ss2.lastIndexOf("\{separate:}");
                    var str = con_ss2.substring(0,index-1);
                    var cmc = str.lastIndexOf("\,");
                    var pep = str.substring(cmc+1,str.length);
                    var xvx = pep.replace("[","");

                    var _str1=con_ss.match(/\_(\d+)/);
                    console.log(_str1[1]);
                    var strc=parseInt(_str1[1])+1;

                    var nums = con_ss.split(':');
                    console.log(nums[1]);
                    console.log(nums[2]);
                    console.log(nums[3]);
                    console.log(nums[4]);

                    var nums1=nums[1];
                    var nums2=nums[2];
                    var nums3=nums[3];
                    var nums4=nums[4];
                    var _x=nums1/(1920/div1);
                    var _y=nums2/(1080/div2);
                    _w=nums3/(1920/div1);
                    _h=nums4/(1080/div2);
                    console.log("X:"+_x);
                    console.log("Y:"+_y);
                    var o_col=0;
                    var o_row=strc/dev_col;//行
                    console.log(strc+"::::::"+dev_col);
                    if(strc<dev_col){
                        o_col=(Math.ceil(o_row)*dev_col)-(dev_col-strc);//列
                        console.log("dev_col:::"+dev_col+":::::"+Math.ceil(o_row));
                        console.log("列:"+o_col+":::strc:"+strc);
                    }else if( strc%dev_col ==0){
                        // 偶数
                        o_col=(Math.ceil(o_row)*dev_col)-(strc-dev_col);//列
                        console.log("dev_col:::"+dev_col+":::::"+Math.ceil(o_row));
                        console.log("列:"+o_col+":::strc:"+strc);
                    }else{
                        // 奇数
                        o_col=(parseInt(strc)+parseInt(dev_row))-(Math.ceil(o_row)*parseInt(dev_col));//列
                        console.log("列:"+o_col+"dev_col:::"+dev_col+":::::"+Math.ceil(o_row)+":::strc:"+strc+"::dev_row:"+dev_row);
                    }

                    console.log("行:"+Math.ceil(o_row));
                    var o_x=((o_col*div1)-div1)+_x;
                    var o_y=((Math.ceil(o_row)*div2)-div2)+_y;
                    console.log("XX:"+o_x);//
                    console.log("YY:"+o_y);//

                    var last="";
                    if(xvx!=con_ss){
                        last=xvx;
                        console.log(last);
                        var _str2=last.match(/\_(\d+)/);
                        console.log(_str2[1]);
                        var strc2=parseInt(_str2[1])+1;

                        var nums2 = last.split(':');
                        console.log(nums2[1]);
                        console.log(nums2[2]);
                        console.log(nums2[3]);
                        console.log(nums2[4]);

                        var _nums1=nums2[1];
                        var _nums2=nums2[2];
                        var _nums3=nums2[3];
                        var _nums4=nums2[4];
                        var _x2=_nums1/(1920/div1);
                        var _y2=_nums2/(1080/div2);
                        var _w2=_nums3/(1920/div1);
                        var _h2=_nums4/(1080/div2);
                        console.log("X:"+_x2);
                        console.log("Y:"+_y2);
                        console.log("W:"+_w2);
                        console.log("H:"+_h2);
                        var o_col2=0;
                        var o_row2=strc2/dev_col;//行
                        console.log(strc+"::::::"+dev_col);
                        if(strc2==1){
                            o_col2=(Math.ceil(o_row2)*dev_col)-(dev_col-strc2);//列
                            console.log("dev_col:::"+dev_col+":::::"+Math.ceil(o_row2));
                            console.log("列:"+o_col2+":::strc:"+strc2);
                        }else if( strc2%dev_col ==0){
                            // 偶数
                            o_col2=(Math.ceil(o_row2)*dev_col)-(strc2-dev_col);//列
                            console.log("dev_col:::"+dev_col+":::::"+Math.ceil(o_row2));
                            console.log("列:"+o_col2+":::strc:"+strc2);
                        }else{
                            // 奇数
                            o_col2=(parseInt(strc)+parseInt(dev_row))-(Math.ceil(o_row)*parseInt(dev_col));//列
                            console.log("列:"+o_col2+":::::"+Math.ceil(o_row2)+":::strc:"+strc2);
                        }

                        console.log("行:"+Math.ceil(o_row2));
                        var o_x2=((o_col2*div1)-div1)+_x2;
                        var o_y2=((Math.ceil(o_row2)*div2)-div2)+_y2;
                        console.log("XX:"+o_x2);
                        console.log("YY:"+o_y2);
                        
                        _w=((o_col2*div1)-div1)+_w2-o_x;
                        _h=((Math.ceil(o_row2)*div2)-div2)+_h2-o_y;
                        console.log("o_col2:"+o_col2+"div2:"+div1+"_h2:"+_w2+"o_y:"+o_x);
                        console.log("o_row2:"+Math.ceil(o_row2)+"div2:"+div2+"_h2:"+_h2+"o_y:"+o_y);
                        console.log("WW:"+_w);
                        console.log("HH:"+_h);
                        if((Math.ceil(o_row2)==Math.ceil(o_row))||((Math.ceil(o_row)!=1)&&(Math.ceil(o_row)==Math.ceil(o_row2)))){
                            console.log("横着跨屏");
                            _w=_w;
                            _h=nums4/(1080/div2);
                        }
                        if((o_col2==o_col)||((o_col!=1)&&(o_col==o_col2))){
                            console.log("竖着跨屏");
                            _w=nums3/(1920/div1);
                            _h=_h;
                        }
                    }
                    var yuanData = '<div id="'+win_id+'" class="tree-xin ui-draggable" style="width: '+_w+'px; height: '+_h+'px; top: '+o_y+'px; position: absolute; left: '+o_x+'px;opacity: 1;margin-bottom: 0px" >' +
                                   '<input type="hidden" id="'+ipc_id+'" name="1280" class="720" value="">' +
                                   '<img id="xin-img" src="../image/叉叉.png" style="width: 23px; height: 23px; position: absolute; top: 0%; right: 0%; font-size: 1.7vw;">'+
                                   '</div>';
                    $('#mqxians').append(yuanData);
                    cont_e+=1;
                }
            }
            $('#mqxians').delegate("#xin-img", "click", function () {
                pip_close = $(this).parent().attr("id");
                $(this).parent().remove();
                setPipWidgetClose();
                alert("111"+pip_close);
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
            $('#mqxians .tree-xin').draggable({
                scope: 'drop',
                containment: '#mqxians',
                revert: 'invalid',
            });
            $('#mqxians').droppable({//拖动到指定的div当中
                scope: 'drop',
                addClasses: false,
                drop: function (event, ui) {//拖拽期间触发
                    var ud = ui.helper;
                    var bgColor="";	//改变背景颜色的函数
                    var colorArray =new Array("0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f");
                    for(var i=0; i<6; i++){
                        bgColor +=colorArray[Math.floor(Math.random()*16)];
                    }
                    var zcx=parseInt($(ud).siblings().last().css("z-index"));
                    var zi=zcx+1;
                    var _timerProvinceClick=null;
                    ud.appendTo($("#mqxians")).css({
                        position : 'absolute',
                        left : ui.offset.left-($('#mqxians').offset().left),
                        top : ui.offset.top-($('#mqxians').offset().top),
                        border: '2px solid #'+bgColor,
                        zIndex:zi,
                    }).draggable({
                        scope : 'drop',
                        revert : 'invalid',
                        containment: 'parent',
                        stop: function(event, ui) {
                            var timeOutEvent=0;
                            //单击显示在最上层
                            $("#mqxians .tree-xin,#mqxians .nav-imgs").off('click').click(function () {
                                var maxZ = Math.max.apply(null,
                                    $.map($('#mqxians .tree-xin,#mqxians .nav-imgs'), function(e,n) {
                                        if ($(e).css('position') != 'static')
                                        return parseInt($(e).css('z-index')) || -1;
                                    })
                                );
                                var zind=maxZ+1;
                                    $(this).css({"z-index": zind + 1});
                                    pip_tree_xin = $(this).attr("id");
                                    pip_W = $(this).outerWidth()+1;
                                    pip_H = $(this).outerHeight()-1;
                                    pip_X = $(this).offset().left+1;
                                    pip_Y = $(this).offset().top+1;
                                    pip_kc_id = $(this).find(' input').attr('id');
                                    pip_w = $(this).find(' input').attr('name');
                                    pip_h = $(this).find(' input').attr('class');
                                    src_val=$(this).find(' input').attr('value');
                                    console.log("66666号：pip_W::" + pip_W + ":pip_H::" + pip_H + ":pip_X::" + pip_X + ":pip_Y::" + pip_Y + ":ID::" + pip_kc_id + ":W:" + pip_w + ":H:" + pip_h);
                                    setPipWidgetChange();
                            });
                            $(event.originalEvent.target).on({
                                touchend: function(e){
                                    clearTimeout(timeOutEvent);
                                    var _value=$(ui.helper.context).find(" input").attr("value");
                                    if (_value==""||_value==null){
                                        var ipc_music="";//
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
                    pip_tree_xin = ud.attr("id");
                    pip_W = $(ud).outerWidth()+2;
                    pip_H = $(ud).outerHeight()+2;
                    pip_X = ud.offset().left;
                    pip_Y = ud.offset().top;
                    pip_kc_id = ud.find(' input').attr('id');
                    pip_w = ud.find(' input').attr('name');
                    pip_h = ud.find(' input').attr('class');
                    src_val=ud.find(' input').attr('value');
                    console.log("五号：pip_W::" + pip_W + ":pip_H::" + pip_H + ":pip_X::" + pip_X + ":pip_Y::" + pip_Y + ":ID::" + pip_kc_id + ":W:" + pip_w + ":H:" + pip_h);
                    setPipWidgetChange();
                    var objW=$(".wrap").width();
                    var objH=$(".wrap").height();
                    ud.mousedown(function(e) {
                        //右键为3
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
                                //关窗
                                setPipWidgetClose();
                            });
                        }
                    });
                    //PC端双击放大
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
                            var div1 = $('td[id^="com"]').outerWidth()+1;//窗口W
                            var div2 = $('td[id^="com"]').outerHeight()+1;//窗口H
                            var dev_row = document.getElementById("input_row").value.trim();
                            var dev_col = document.getElementById("input_col").value.trim();
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
                    //移动端双击放大·缩小
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
                                    var div1 = $('td[id^="com"]').outerWidth()+1;//窗口W
                                    var div2 = $('td[id^="com"]').outerHeight()+1;//窗口H
                                    var dev_row = document.getElementById("input_row").value.trim();
                                    var dev_col = document.getElementById("input_col").value.trim();
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
                    //移动端长按拖拽删除
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
                                        tolerance : "touch",
                                        drop : function(event, ui) {
                                            var udc = ui.draggable;
                                            pip_close=udc.attr("id");
                                            $(udc).remove();
                                            $("#del-shan").hide();
                                            setPipWidgetClose();
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
                                if(zwc<197){
                                    zwc=197;
                                }
                                if(zhc<106){
                                    zhc=106;
                                }
                                
                                mqxians_Img.style.width=zwc+"px";
                                mqxians_Img.style.height=zhc+"px";
                                mqxians_Img.css("left",zxc+"px");
                                mqxians_Img.css("top",zyc+"px");

                                pip_tree_xin=ud.attr("id");
                                pip_W=$(mqxians_Img).outerWidth()+2;
                                pip_H=$(mqxians_Img).outerHeight()+2;
                                pip_X=mqxians_Img.offset().left;
                                pip_Y=mqxians_Img.offset().top;
                                pip_kc_id=mqxians_Img.find(' input').attr('id');
                                pip_w=mqxians_Img.find(' input').attr('name');
                                pip_h=mqxians_Img.find(' input').attr('class');
                                src_val=mqxians_Img.find(' input').attr('value');
                                console.log("2525号：当前窗口::"+pip_tree_xin+"pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                                setPipWidgetChange();
                            }

                            //第二种：缩小
                            if(parseInt(xx) == parseInt($(ud).offset().left) && parseInt(yy) == parseInt($(ud).offset().top) && parseInt(ww) == parseInt($(ud).outerWidth()) && parseInt(hh) == parseInt($(ud).outerHeight())){
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
                                if(zwc<197){
                                    zwc=197;
                                }
                                if(zhc<106){
                                    zhc=106;
                                }

                                mqxians_Img.style.width=zwc+"px";
                                mqxians_Img.style.height=zhc+"px";
                                mqxians_Img.css("left",zxc+"px");
                                mqxians_Img.css("top",zyc+"px");

                                pip_tree_xin=ud.attr("id");
                                pip_W=$(mqxians_Img).outerWidth()+2;
                                pip_H=$(mqxians_Img).outerHeight()+2;
                                pip_X=mqxians_Img.offset().left;
                                pip_Y=mqxians_Img.offset().top;
                                pip_kc_id=mqxians_Img.find(' input').attr('id');
                                pip_w=mqxians_Img.find(' input').attr('name');
                                pip_h=mqxians_Img.find(' input').attr('class');
                                src_val=mqxians_Img.find(' input').attr('value');
                                console.log("2424号：当前窗口::"+pip_tree_xin+"pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                                setPipWidgetChange();
                            }

                            //第三种：只靠左上角，
                            if(parseInt(xx) == parseInt($(ud).offset().left) || parseInt(yy) == parseInt($(ud).offset().top)){
                                // 最大缩放比例限制
                                if (newScale > 2) {
                                    newScale = 2;
                                }
                                // 记住使用的缩放值
                                store.scale = newScale;

                                zwc = ww-parseInt(upper_right_X)-parseInt($(mqxians_Img).outerWidth());
                                zhc = hh-parseInt(lower_left_Y)-parseInt($(mqxians_Img).outerHeight());
                                if(zwc==895||zhc==391){
                                    zwc=909;zhc=405;
                                }
                                mqxians_Img.style.width=zwc+"px";
                                mqxians_Img.style.height=zhc+"px";

                                pip_tree_xin=ud.attr("id");
                                pip_W=$(mqxians_Img).outerWidth()+2;
                                pip_H=$(mqxians_Img).outerHeight()+2;
                                pip_X=mqxians_Img.offset().left;
                                pip_Y=mqxians_Img.offset().top;
                                pip_kc_id=mqxians_Img.find(' input').attr('id');
                                pip_w=mqxians_Img.find(' input').attr('name');
                                pip_h=mqxians_Img.find(' input').attr('class');
                                src_val=mqxians_Img.find(' input').attr('value');
                                console.log("2323号：当前窗口::"+pip_tree_xin+"pip_W::"+pip_W+":pip_H::"+pip_H+":pip_X::"+pip_X+":pip_Y::"+pip_Y+":ID::"+pip_kc_id+":W:"+pip_w+":H:"+pip_h);
                                setPipWidgetChange();
                            }

                            //第四种：只靠右上角，
                            /*if(parseInt(yy) == parseInt($(ud).offset().top) && parseInt(upper_right_X) == parseInt(upper_right_X2)){
                                // 最大缩放比例限制
                                if (newScale > 2) {
                                    newScale = 2;
                                }
                                // 记住使用的缩放值
                                store.scale = newScale;

                                zwc = ww-parseInt($(mqxians_Img).outerWidth());
                                zhc = hh-parseInt(lower_left_Y)-parseInt($(mqxians_Img).outerHeight());
                                mqxians_Img.style.width=zwc+"px";
                                mqxians_Img.style.height=zhc+"px";
                                mqxians_Img.css("left",zwc+"px");
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
        }
    });
}
//开始预案轮巡
function setSceneLoop() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SceneLoop\r\nloop_scene : '+loop_scene+'\r\ntime : '+time+'\r\n',
        async:true,
        success:function (data) {

        }
    });
}
//当前轮巡到的id
function setSceneState() {
    var sec_id=$(this).find('id');
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SceneState\r\ndata : '+sec_id+'\r\n',
        async:true,
        success:function (data) {
        }
    })
}
// 停止轮巡
function setSceneLoopStop(){
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SceneLoopStop\r\n',
        async:true,
        success:function (data) {
        }
    });
}
// 预案删除
function setSceneDel() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SceneDel\r\nscene_id : '+del_id+'\r\n',
        async:true,
        success:function (data) {
        }
    });
}
// 关闭窗口
function setPipWidgetClose() {
    console.log("关闭窗口"+pip_close);
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : PipWidgetClose\r\nwin_id : '+pip_close+'\r\n',
        async:true,
        success:function (data) {
        }
    });
}
//全部关闭
function setPipWidgetAllClose() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : PipWidgetAllClose\r\n',
        async:true,
        success:function (data) {
        }
    });
}
/**
 * 设置多屏组
 */
function setSetScreenGroup() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : SetScreenGroup\r\nGroupId : '+groupId+'\r\nGroupName : '+groupName+'\r\nBeginId : '+beginId+'\r\nEndId : '+endId+'\r\nrow : '+row+'\r\ncol : '+col+'\r\n',
        async:true,
        success:function (data) {

        }
    });
}
/**
 * 删除多屏组
 */
function setDelScreenGroup() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : DelScreenGroup\r\nGroupId : '+groupdelete+'\r\n',
        async:true,
        success:function (data) {

        }
    })
}
/**
 * 修改多屏组名称
 * @constructor
 */
function setModifyScreenGroup() {
    $.ajax({
        type:'post',
        url:'../cgi-bin/web_function.cgi',
        data:'cmd : DelScreenGroup\r\nGroupId : '+modifyscreengroupId+'\r\nGroupName : '+modifyscreengroupName+'\r\n',
        async:true,
        success:function (data) {

        }
    })
}
/**
 * 添加IPC
 */
function setAddIpc() {
    $.ajax({
        type: "post",
        url: '../cgi-bin/web_function.cgi',
        data: 'cmd : AddIpc\r\nipc_'+add_id+' : '+add_ipc+'\r\n',
        async: false,
        success: function (data) {

        },
    });
}
/**
 * 删除IPC
 */
function setDelIpc() {
    $.ajax({
        type: 'post',
        url: '../cgi-bin/web_function.cgi',
        data: 'cmd : DelIpc\r\nipc_'+del_id+' : '+del_ipc+'\r\n',
        async: true,
        success: function (data) {

        }
    });
}

/**
 *清空IPC
 */
function setClearIpc() {
    $.ajax({
        type: 'post',
        url: '../cgi-bin/web_function.cgi',
        data: 'cmd : ClearIpc\r\n',
        async: true,
        success: function (data) {

        }
    });
}
