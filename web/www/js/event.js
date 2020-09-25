$(function () {
    $(".box-list1").click(function () {
        $(".box-list").removeClass("menu_selected");
        $(".box-list1").addClass('menu_selected');
        $('.conf_info').hide();
        $('.dns_conf').show();
        $('.button_list').hide();
        $('.conf_button').show();
    });
    $(".box-list2").click(function () {
        $(".box-list").removeClass("menu_selected");
        $(".box-list2").addClass('menu_selected');
        $('.conf_info').hide();
        $('.balance_conf').show();
        $('.button_list').hide();
        $('.conf_button').show();
    });
    $(".box-list3").click(function () {
        $(".box-list").removeClass("menu_selected");
        $(".box-list3").addClass('menu_selected');
        $('.conf_info').hide();
        $('.rtsp_conf').show();
        $('.button_list').hide();
        $('.conf_button').show();
    });
    $(".box-list4").click(function () {
        $(".box-list").removeClass("menu_selected");
        $(".box-list4").addClass('menu_selected');
        $('.conf_info').hide();
        $('.log_conf').show();
        $('.button_list').hide();
        $('.conf_button').show();
    });
    $(".box-list5").click(function () {
        $(".box-list").removeClass("menu_selected");
        $(".box-list5").addClass('menu_selected');
        $('.conf_info').hide();
        $('.url_list').show();
        $('.button_list').hide();
        $('.url_button').show();
    });
})