jQuery(document).ready(function(){

    var newTopWidth = $('#new_top').width();
    var input = $('input#s');
    var divInput = $('div.input');
    var width = divInput.width();
    var outerWidth = 230;
    var txt = input.val();
    var originalWidth = 250;
    
    $('#search_icon').bind('click', function(){
        if ($("#comboDistricts").css("visibility") != "hidden") {
            $('#bairroButton').click();
        }
        if ($("#top_categories").is(":visible") == true) {
            $('#filtroButton').click();
        }
        $("#new_top #container").show();
        divInput.show();
        input.hide();
        $(this).hide();
        divInput.animate({
            width: outerWidth + 'px'
        }, 300, function(){
            input.show();
            input.focus();
        });
        $("#new_top #busca").animate({
            width: '250px'
        }, 300);
        
    });
    
    $('#submit_icon').live('click', function(){
        $("#new_top #busca").animate({
            width:'45px'
        }, 300)
        $(this).parent().animate({
            width: width + 'px'
        }, 300).removeClass('focus');
        $("#new_top #container").hide();
        $('#search_icon').show();
    });

    $("#ir_submit").bind('click', function(){
        $('#searchForm').submit();
    });
    
    $('#searchForm').submit(function(){
        $.openPopupLayer({
            name: 'search_causes',
            width: 670,
            url: "/search_causes",
            parameters: {
                search: input.val()
            }
        });
				
        return false;
    });
    
});
