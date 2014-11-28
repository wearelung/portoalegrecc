var tweets;
var currentTweetIndex;
var totalTweets;
var timerTwitter;
	
$(function(){
	$('.scroll-pane').jScrollPane();
	
	bindEvents();
	onScroll();
	getTweets();
	
	timerTwitter = setInterval(function() {
		getTweets();
	}, 60000);
	
	$('#maps').bind('click', function() {
		$('#comboDistricts').addClass('hidden');
	});
});

$(window).scroll(function() {
	onScroll();
});

$(window).resize(function() {
	onScroll();
});

function onScroll() {
	if (navigator.userAgent.indexOf('Mobile') != -1) {
		var top = $('#top');
		var footer = $('#footer');
		var combo = $('#comboDistricts');
		var innerHeight = $('#inner').height();
		var footerHeight = footer.height();
		var scrollTop = $(document).scrollTop();
		var scrollLeft = $(document).scrollLeft();
		
		var x = (scrollLeft * -1);
		var yFooter = (scrollTop * -1);
		
		footer.animate({
				bottom: yFooter + 'px',
				right: x  + 'px'
			},
			{
				duration: 500,
				queue: false
			}
		);
		
		top.animate({
				top: scrollTop + 'px',
				right: x + 'px'
			},
			{
				duration: 500,
				queue: false
			}
		);
		
		combo.animate({
				top: (scrollTop + 32) + 'px',
				right: (x + 5) + 'px'
			},
			{
				duration: 500,
				queue: false
			}
		);
		
		if ($('.modalBox').length > 0) {
			var newLeft = ($(window).width()/2) - ($('.modalBox').width()/2) + (x * -1);
			$('.modalBox').parent().animate({
					left: newLeft + 'px'
				},
				{
					duration: 500,
					queue: false
				}
			);
		}
		
		if ($('.modalBox2').length > 0) {
			var newLeft = ($(window).width()/2) - ($('.modalBox2').width()/2) + (x * -1);
			$('.modalBox2').parent().animate({
					left: newLeft + 'px'
				},
				{
					duration: 500,
					queue: false
				}
			);
		}
	}
}

function modalSize(size){ // altera o tamanho do modal
	var modalH = $('#colorbox').height();
	$('#cboxLoadedContent,#cboxContent,#cboxWrapper,#colorbox').height(modalH+size);
}

function bindEvents() {
	$('#left #redesSociais a').bind('click', function() {
		$(this).parents('ul:first').find('li').attr('class', '');
		$(this).parent().addClass('selected');
		
		$('#left .social-media-content').addClass('hide');
		$('#left .social-media-' + $(this).attr('class')).removeClass('hide');
		
		GA.trackEvent('links_externos', 'clique', $(this).attr('class'));
	});
	
	$('#left .social-media-twitter .prev').bind('click', function() {
		if (currentTweetIndex > 0) {
			getTweet(currentTweetIndex-1);
		}
	});
	
	$('#left .social-media-twitter .next').bind('click', function() {
		if (currentTweetIndex < totalTweets - 1) {
			getTweet(currentTweetIndex+1);
		}
	});
}

function getTweets() {
	var tweetsErrorCallback = function(errorMessage, tweet){
		var error = 'Error: Twitter Rate Limit Exceeded!';
		alert(error);
	}
	
	var tweetsSuccessCallback = function(data){
		if (data && data.results && data.results.length) {
			tweets = data.results;
			totalTweets = tweets.length;
			getTweet(0);
		}
	}
	
	var tweetsData = Twitter.tweets();
	tweetsData.containing('#portoalegrecc').or('#poacc').or('@portoalegrecc').limit(50).all(tweetsSuccessCallback, tweetsErrorCallback);
}

function linkUsernames(text) {
    return text.replace(/@([a-z,A-Z,0-9,_,-]+)/g, '<a href="http://twitter.com/$1" target="_blank">@$1</a>');
}

function linkHashtags(text) {
    return text.replace(/#([a-z,A-Z,0-9,_,-]+)/g, '<a href="http://twitter.com/#search?q=$1" target="_blank">#$1</a>');
}

function linkUrls(text) {
	return text.replace(/(\b(https?|ftp|file):\/\/[-A-Z0-9+&@#\/%?=~_|!:,.;]*[-A-Z0-9+&@#\/%=~_|])/ig, '<a href="$1" target="_blank">$1</a>');
//	return text.replace(/(https?:\/\/([-\w\.]+)+(:\d+)?(\/([\w\/_\.]*(\?\S+)?)?)?)/g, '<a href="$1" target="_blank">$1</a>');
}

function relTime(time_value) {
	time_value = time_value.replace(/(\+[0-9]{4}\s)/ig,"");
	var parsed_date = Date.parse(time_value);
	var relative_to = (arguments.length > 1) ? arguments[1] : new Date();
	var timeago = parseInt((relative_to.getTime() - parsed_date) / 1000);
	
	if (timeago < 60) return 'menos que 1 minuto atrás';
	else if(timeago < 120) return '1 minuto atrás';
	else if(timeago < (45*60)) return (parseInt(timeago / 60)).toString() + ' minutos atrás';
	else if(timeago < (90*60)) return '1 hora atrás';
	else if(timeago < (24*60*60)) return (parseInt(timeago / 3600)).toString() + ' horas atrás';
	else if(timeago < (48*60*60)) return '1 dia atrás';
	else return (parseInt(timeago / 86400)).toString() + ' dias atrás';
}

function getTweet(index) {
	clearInterval(timerTwitter);
	timerTwitter = setInterval(function() {
		getTweets();
	}, 60000);
	
	currentTweetIndex = index;
	var data = tweets[index];
	var text = linkUrls(data.text);
	text = linkUsernames(text);
	
	$('#left .social-media-twitter .photo').attr('src', data.profile_image_url);
	$('#left .social-media-twitter .fullname').html(data.from_user);
//	$('#left .social-media-twitter .locale').attr('src', '');
	$('#left .social-media-twitter .tweet').html(linkHashtags(text));
	$('#left .social-media-twitter .time').html(relTime(data.created_at));
	$('#left .social-media-twitter .button-reply').attr('href', 'http://twitter.com/?status=@'+ data.from_user +'&in_reply_to_status_id='+ data.id +'&in_reply_to='+ data.from_user);
	$('#left .social-media-twitter .button-rt').attr('href', 'http://twitter.com/home?status=' + 'RT @'+ data.from_user +' ' + data.text);

	if (index == 0) {
		$('#left .social-media-twitter .prev').hide();
		
		if (totalTweets == 1) {
			$('#left .social-media-twitter .next').hide();
		} else {
			$('#left .social-media-twitter .next').show();
		}
	} else {
		$('#left .social-media-twitter .prev').show();
		
		if (index == totalTweets-1) {
			$('#left .social-media-twitter .next').hide();
			$('#left .social-media-twitter .prev').addClass('single');
		} else {
			$('#left .social-media-twitter .prev').removeClass('single');
			$('#left .social-media-twitter .next').show();
		}
	}
}

function goToMap(value) {
	var url = 'http://'+location.host;
	
	if (value && value != '') {
		url += '?q=' + value;
	}
	
	window.location = url;
}

function sendForm(button, additionalData, validationMessages) {
	var data = {};
	var error = false;
	var form = $(button).parents('form:first');
	var action = form.attr('action');
	var inputs = form.find(':input[name]');
	var messageBox = $('#feedbackMessage');
	var messageSuccess = 'Formulário enviado com sucesso.';
	var messageValidationError = 'Por favor preencha todos os campos.';
	var messageWait = 'Por favor aguarde, o formulário está sendo enviado...';
	var messageServerError = 'Erro no server.';
	
	inputs.removeClass('error');
	
	for (var i = 0; i < inputs.length; i++) {
		if ($(inputs[i]).hasClass('required') && (inputs[i].value == '' || inputs[i].value == validationMessages[i])) {
			error = true;
			$(inputs[i]).addClass('error');
			$(inputs[i]).val(validationMessages[i]);
			$(inputs[i]).focus(function() {
				var input = $(this);
				input.val('')
				input.removeClass('error');
			});
		}
		
		data[inputs[i].name] = inputs[i].value;
	}
	
	if (additionalData) {
		for (var k in additionalData) {
			if (additionalData[k] && additionalData[k] != '') {
				data[k] = additionalData[k];
			}
		}
	}
	
	if (error) {
		messageBox.css({
			color: 'red',
			fontSize: '13px'
		}).html(messageValidationError);
		
		return;
	}
	
	messageBox.css({
		color: '#7DBD00',
		fontSize: '13px'
	}).html(messageWait);
	
	$.ajax({
		data: data,
		type: 'POST',
		url: action,
		success: function(){
			inputs.val('');
			messageBox.css({
				color: '#7DBD00',
				fontSize: '13px'
			}).html(messageSuccess);
		},
		error: function() {
			messageBox.css({
				color: 'red',
				fontSize: '13px'
			}).html(messageServerError);
		}
	});	
}

/*******************
 * GoogleAnalytics *
 ******************/
function GoogleAnalytics (UA) {
  GAnalytics = _gat._getTracker(UA);
  GAnalytics._setDomainName('none');
  GAnalytics._setAllowLinker(true);
  GAnalytics._initData();
}

GoogleAnalytics.prototype.track = function (action) {
	return (action) ? GAnalytics._trackPageview(action) : GAnalytics._trackPageview();
}

GoogleAnalytics.prototype.trackEvent = function (category, action, opt_value) {
	return _gaq.push(['_trackEvent', category, action, opt_value]);
}

GoogleAnalytics.prototype.trackRedirect = function (action, url) {
  this.track(action);
  setTimeout(function () {
	location.href = url;
  }, 1000);
}