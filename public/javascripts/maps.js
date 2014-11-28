
var map;

var cookieList = function(cookieName) {
	var cookie = $.cookie(cookieName);
	var items = cookie ? cookie.split(/,/) : new Array();
	return {
	    "add": function(val) {
	        items.push(val);
	        $.cookie(cookieName, items.join(','));
	    },
	    "remove" : function(val) {
	    	var i = items.indexOf(val);
	    	if(i != -1) items.splice(i, 1);
	    	$.cookie(cookieName, items.join(','));
	    },
	    "has" : function(val) {
	    	var i = items.indexOf(val);
	    	return i != -1;
	    },
	    "clear": function() {
	        items = null;
	        $.cookie(cookieName, null);
	    },
	    "items": function() {
	        return items;
		}
	}
}

function initialize() {

}

var Map = Class.extend({
	init: function() {
		this.requestTimer = null;
		this.markersList = {};
		this.createMap();
		this.overMarker = new OverMarker(this.portoalegreLocation, {}, this, false);
		this.createCauseButton = $('#createCauseButton');
		this.loginButton = $('#loginButton');
		this.createZoom();
		this.bindEvents();
		if (lu == false) {
			$.cookie('waitingToAddMarker', null)
		} else if ($.cookie('waitingToAddMarker') != null) {
			this.addCause();
		}
	},
	
	getMapContainerId: function() {
		return 'maps';
	},
	
	loadData: function(data) {
		var _this = this;
		
		$.ajax({
			url: 'causes/visibles',
			data: data,
			dataType: 'json',
			success: function(result) {
				_this.populateMap(result);
			}
		})
	},
	
	populateMap: function(result) {
	//	this.removeAllMarkers();
		for (var i = 0; i < result.length; i++) {
			result[i]['cause'].latLng = [result[i]['cause'].latitude, result[i]['cause'].longitude];
			//result[i]['cause'].category_name = result[i]['category'].name;
			
			if (typeof(this.markersList[result[i]['cause'].id]) == 'undefined') {
				var m = new CauseMarker(result[i]['cause'].latLng, result[i]['cause'], this, true);
				this.addMarker(m);	
			}
		}
	},

	setCenterByCurrentLocation: function(position){
		var self=this;
		if (navigator.geolocation)
    	{
    		navigator.geolocation.getCurrentPosition(function(position){
    			self.portoalegreLocation = new google.maps.LatLng( position.coords.latitude,  position.coords.longitude);
				self.map.panTo(self.portoalegreLocation);
    		});
    	}	
	},
	createMap: function() {
		var startLat = $.cookie("map.lat") != null ? $.cookie("map.lat") : -30.06018867580871;
		var startLng = $.cookie("map.lng") != null ? $.cookie("map.lng") : -51.2034991693115;
		var startZoom = $.cookie("map.zoom") != null ? $.cookie("map.zoom") : 13;
		startZoom = parseInt(startZoom);
		
		this.portoalegreLocation = new google.maps.LatLng(startLat, startLng);
		//this.setCenterByCurrentLocation(); //Ocorreu bug para pessoa de fora de Porto Alegre, poriço estamos comentando
		var MY_MAPTYPE_ID = 'portoalegre.cc';

		this.map = new google.maps.Map(document.getElementById(this.getMapContainerId()), {
			zoom: startZoom,
			center: this.portoalegreLocation,
			/*
			mapTypeControlOptions: {
				mapTypeIds: [google.maps.MapTypeId.ROADMAP, MY_MAPTYPE_ID]
			},
			*/
			mapTypeId: MY_MAPTYPE_ID,
			streetViewControl: false,
			navigationControl: false,
			scrollwheel: false,
			mapTypeControl: false
		});

		var styledMapOptions = {
			name: MY_MAPTYPE_ID
		};
		
		var poaccMapType = new google.maps.StyledMapType(this.getStyle(), styledMapOptions);
		this.map.mapTypes.set(MY_MAPTYPE_ID, poaccMapType);
		
		this.map.mapTypes[MY_MAPTYPE_ID].minZoom = MIN_ZOOM;
		this.map.mapTypes[MY_MAPTYPE_ID].maxZoom = MAX_ZOOM;
		/*
		google.maps.event.addListener(google.maps.InfoWindow, 'domready', function(ev) {
			alert('a');
		});
		*/
		
		
		var moveElements = function() {
			$('#maps > div > div:eq(1)').css({
				left: '250px'
			});
			
			$('#maps > div > div:eq(2)').css({
				bottom: '30px'
			});
		}
		
		var adjustGoogleCopyright = function() {
			if ($('#maps > div > div:eq(2)').length > 0) {
				moveElements();
			} else {
				setTimeout(adjustGoogleCopyright, 2500);	
			}
		}
		
		adjustGoogleCopyright();
		
		$(window).resize(function() {
			setTimeout(moveElements, 100);
		});
	},
	
	createZoom: function() {
		var _this = this;
		var startZoom = $.cookie("map.zoom") != null ? $.cookie("map.zoom") : 13;
		
		startZoom = parseInt(startZoom);

		$("#slider-vertical").slider({
			orientation: "vertical",
			range: "min",
			min: MIN_ZOOM,
			max: MAX_ZOOM,
			value: startZoom,
			slide: function( event, ui ) {
				_this.zoom(ui.value);
				/*$( "#amount" ).val( ui.value );*/
			}
		});
		
		$("#zoom-in").bind('click', function() {
			var currentZoom = _this.map.getZoom();
			
			if (currentZoom < MAX_ZOOM) {
				_this.zoom(currentZoom + 1);
			}
		});
		
		$("#zoom-out").bind('click', function() {
			var currentZoom = _this.map.getZoom();
			
			if (currentZoom > MIN_ZOOM) {
				_this.zoom(currentZoom - 1);
			}
		});
		
		if (navigator.userAgent.indexOf('Mobile') != -1 && $("#slider-vertical").length > 0) {
			$("#slider-vertical").addTouch();
		}
	},
	
	zoom: function(value) {
		this.map.setZoom(value);
		$.cookie("map.zoom", value);		
	},
	
	getStyle: function() {
		return [{
			featureType: "all",
			elementType: "all",
			stylers:  [
				{ invert_lightness: true },
				{ visibility: "on" },
				{ saturation: 10 },
				{ lightness: 30 },
				{ gamma: 0.5 },
				{ hue: "#435158" }]
		    },
			{
			featureType: "water",
			elementType: "all",
			stylers: [
				{ hue: "#00AAFF" },
				{ lightness: 30 },
				{ saturation: 20 }]
		}];
	},
	
	getMap: function() {
		return this.map;
	},
	
	bindEvents: function() {
		var _this = this;
		google.maps.event.addListener(this.map, 'click', function(ev) {
			if ($.cookie('waitingToAddMarker') != null) {
				if (!lu) {
					if (collect_email) {
            			openLogin('collect_email');
          			} else {
            			openLogin();
          			}
					return false;
				}
				
				_this.createCauseButton.removeClass('clicked');
				$.cookie('waitingToAddMarker', null);
				
 				var local = '';
 				var district = '';
				var geocoder = new google.maps.Geocoder();
				geocoder.geocode({ 'latLng': ev.latLng}, function(results, status) {
					if (status == google.maps.GeocoderStatus.OK) {
						if (results.length > 0) {
							var local = results[0].formatted_address;
							var district, city, state;

							for (var i = 0; i < results[0].address_components.length; i++) {
								if (results[0].address_components[i].types[0] == 'sublocality') {
									district = results[0].address_components[i]['short_name'];
								} else if (results[0].address_components[i].types[0] == 'locality') {
									city = results[0].address_components[i]['short_name'];
								} else if (results[0].address_components[i].types[0] == 'administrative_area_level_1') {
									state = results[0].address_components[i]['short_name'];
								}
							}
						}
 					}
					if (city == 'Porto Alegre' && state == 'RS') {		
						$.openPopupLayer({
					        name: 'mdlEscrever',
					        width: 710,
					        url: 'causas/new',
							parameters: {
								lat: ev.latLng.lat(),
								lng: ev.latLng.lng(),
								local: local,
								district: district
							},
							success: function() {
								$('#newCauseForm input[name=title]').focus();
								$("#left").height("1500px");
								$("#inner").height("1500px");
                				google.maps.event.trigger(_this.map, 'resize');
                				$(window).resize();
							}
						});
					}
				});
			}
		});
		
		google.maps.event.addListener(this.map, 'zoom_changed', function(ev) {
			$("#slider-vertical").slider({value:_this.map.getZoom()});
			$.cookie("map.zoom", _this.map.getZoom());
			_this.removeAllMarkers();
		});
		
		google.maps.event.addListener(this.map, 'bounds_changed', function(ev) {		
			clearTimeout(_this.requestTimer);
			var categories = new cookieList("categories");
			_this.requestTimer = setTimeout(function(){
				var data = {
						topLeftY: _this.map.getBounds().getNorthEast().lat(),
						topLeftX: _this.map.getBounds().getNorthEast().lng(),
						bottomRightY: _this.map.getBounds().getSouthWest().lat(),
						bottomRightX: _this.map.getBounds().getSouthWest().lng(),
						currentZoom: _this.map.getZoom() - MIN_ZOOM,
						maxZoom: MAX_ZOOM-MIN_ZOOM,
						cats: categories.items()
					};
					_this.removeAllMarkers();				
					_this.loadData(data);					
			},500); 
		});
		
		google.maps.event.addListener(this.map, 'dragend', function(ev) {
			var geocoder = new google.maps.Geocoder();
			var latLng = _this.map.getCenter();

			geocoder.geocode({ 'latLng': latLng}, function(results, status) {
				if (status == google.maps.GeocoderStatus.OK) {
					if (results.length > 0) {
						var local = results[0].formatted_address;
						var district, city, state;
						
						for (var i = 0; i < results[0].address_components.length; i++) {
							if (results[0].address_components[i].types[0] == 'sublocality') {
								district = results[0].address_components[i]['short_name'];
							} else if (results[0].address_components[i].types[0] == 'locality') {
								city = results[0].address_components[i]['short_name'];
							} else if (results[0].address_components[i].types[0] == 'administrative_area_level_1') {
								state = results[0].address_components[i]['short_name'];
							}
						}
					}
				}
				
				if (city == 'Porto Alegre' && state == 'RS') {		
			        $.cookie("map.lat", latLng.lat());
					$.cookie("map.lng", latLng.lng());
				} else {
					if ($.cookie("map.lat") == null && $.cookie("map.lng") == null) {
						$.cookie("map.lat", _this.portoalegreLocation.lat());
						$.cookie("map.lng", _this.portoalegreLocation.lng());
					}
					
					var position = new google.maps.LatLng($.cookie("map.lat"), $.cookie("map.lng"));
					_this.map.panTo(position);
				}
			});
		});
		
		this.loginButton.bind('click', function() {
			openLogin();
		});
		
		this.createCauseButton.bind('click', function() {
			$.cookie('waitingToAddMarker', 'true');

			if (!lu) {
				if (collect_email) {
  					openLogin('collect_email');
				} else {
			    	openLogin();
        		}
				return false;
			}
			
			_this.addCause();
		});
	},
	
	addCause: function() {
		this.createCauseButton.addClass('clicked');
	},
	
	createMarker: function(data) {
		var testMarker = new CauseMarker(data.latLng, data, this, true);

		this.addMarker(testMarker);
	},
	
	addMarker: function(marker, avoidAttachEvent) {			
		this.markersList[marker.id] = marker;
		
		if (!avoidAttachEvent) {
			marker.onAttach();
		}
	},
	
	removeAllMarkers: function() {
		for (var i in this.markersList) {
			this.markersList[i].getMarker().setMap(null);
			delete(this.markersList[i]);
		}
		
		this.markersList = {};
	},
	
	categorize: function(categories) {
		var _this = this;
		clearTimeout(_this.requestTimer);
		this.requestTimer = setTimeout(function(){
			var data = {
					topLeftY: _this.map.getBounds().getNorthEast().lat(),
					topLeftX: _this.map.getBounds().getNorthEast().lng(),
					bottomRightY: _this.map.getBounds().getSouthWest().lat(),
					bottomRightX: _this.map.getBounds().getSouthWest().lng(),
					currentZoom: _this.map.getZoom() - MIN_ZOOM,
					maxZoom: MAX_ZOOM-MIN_ZOOM,
					cats: categories
				};
				_this.removeAllMarkers();
				_this.loadData(data);
		},500);
	},
	
	search: function(strSearch) {
		if (strSearch == 'Centro') {
			strSearch = [-30.033916, -51.229162];
		} else if (strSearch == 'Bom Fim') {
			strSearch = [-30.032589914500445, -51.21292029564819];
		} else if (strSearch == 'Rio Branco') {
			strSearch = [-30.034362, -51.199036];
		}
		
		if (strSearch && strSearch != '') {
			if (typeof(strSearch) == 'string') {
				var geocoder = new google.maps.Geocoder();
				var _this = this;
				strSearch = strSearch + ', Porto Alegre, Brasil';

				geocoder.geocode({'address': strSearch}, function(results, status) {
					if (status == google.maps.GeocoderStatus.OK) {

						if (results.length > 0) {
							var local = results[0].formatted_address;
							var district, city, state;

							for (var i = 0; i < results[0].address_components.length; i++) {
								if (results[0].address_components[i].types[0] == 'sublocality') {
									district = results[0].address_components[i]['short_name'];
								} else if (results[0].address_components[i].types[0] == 'locality') {
									city = results[0].address_components[i]['short_name'];
								} else if (results[0].address_components[i].types[0] == 'administrative_area_level_1') {
									state = results[0].address_components[i]['short_name'];
								}
							}
						}
					}
					
					if (city == 'Porto Alegre' && state == 'RS') {		
				        $.cookie("map.lat", results[0].geometry.location.lat());
						$.cookie("map.lng", results[0].geometry.location.lng());
						
						_this.map.panTo(results[0].geometry.location);
						_this.zoom(15);	
					} else {
						if ($.cookie("map.lat") == null && $.cookie("map.lng") == null) {
							$.cookie("map.lat", _this.portoalegreLocation.lat());
							$.cookie("map.lng", _this.portoalegreLocation.lng());
						}

						var position = new google.maps.LatLng($.cookie("map.lat"), $.cookie("map.lng"));
						_this.map.panTo(position);
					}
				});
			} else {
				var position = new google.maps.LatLng(strSearch[0], strSearch[1]);

				this.map.panTo(position);
				$.cookie("map.lat", strSearch[0]);
				$.cookie("map.lng", strSearch[1]);
				this.zoom(15);
			}
		}
	}
});

var SmallMap = Map.extend({
	init: function(startLat, startLng) {
		this.startLat = startLat;
		this.startLng = startLng;
		this.inherited().init();
	},
	
	getMapContainerId: function() {
		return 'small_map';
	},
	
	createMap: function() {
		this.createSmallMap();
	},
	
	createSmallMap: function() {		
		this.portoalegreLocation = new google.maps.LatLng(this.startLat, this.startLng);
		var MY_MAPTYPE_ID = 'portoalegre.cc';

		this.map = new google.maps.Map(document.getElementById(this.getMapContainerId()), {
			zoom: 15,
			center: this.portoalegreLocation,
			mapTypeId: MY_MAPTYPE_ID,
			mapTypeControl: false,
			streetViewControl: false,
			navigationControl: false,
			scaleControl: false,
			draggable: false
		});

		var styledMapOptions = {
			name: MY_MAPTYPE_ID
		};
		
		var poaccMapType = new google.maps.StyledMapType(this.getStyle(), styledMapOptions);
		this.map.mapTypes.set(MY_MAPTYPE_ID, poaccMapType);
		
		this.map.mapTypes[MY_MAPTYPE_ID].minZoom = MIN_ZOOM;
		this.map.mapTypes[MY_MAPTYPE_ID].maxZoom = MAX_ZOOM;
	},
	
	bindEvents: function() {
		google.maps.event.addListener(this.map, 'click', function(ev) {
			goToMap();
		});
	}
});

var Marker = Class.extend({
	init: function(position, data, parent, attachToMap) {
		this.position = position;
		
		if (position instanceof Array) {
			this.position = new google.maps.LatLng(position[0], position[1]);
		} else {
			this.position = position;
		}
		
		this.id = data.id;
		this.data = data;
		this.parent = parent;
		this.geocoder = new google.maps.Geocoder();
		
		var markerImage;

		var markerOptions = this.getIconProperties();
		
		if (typeof(markerOptions) == 'string') {
			markerImage = markerOptions;
		} else {
			markerImage = new google.maps.MarkerImage(markerOptions[0], markerOptions[1], markerOptions[2], markerOptions[3]);
		}
		
		this.marker = new google.maps.Marker({
			position: this.position,
			map: attachToMap ? this.parent.getMap() : null,
			icon: markerImage,
			zIndex: 5,
			title: data.category_name + ' / ' + data.title
		});
	},
	
	getAddress: function() {
		this.geocoder.geocode( { 'latLng': this.position}, function(results, status) {
			if (status == google.maps.GeocoderStatus.OK) {
				if (results.length > 0) {
					return (results[0].formatted_address);
				}
			}
			
			return null;
		});
	},
	
	getMarker: function() {
		return this.marker;
	},
	
	getIconProperties: function() {
		return 'img/pin-01.png';
	},
	
	onAttach: function() {
		this.bindEvents();
	},
	
	bindEvents: function() {
		var _this = this;
	}
});
	
var CauseMarker = Marker.extend({
	bindEvents: function() {
		this.inherited().bindEvents();
		
		var _this = this;
		google.maps.event.addListener(this.marker, 'click', function(ev) {
			location = _this.data.url;
		});
	},
	
	getIconProperties: function() {
		return '/img/pin_' + this.data.category_id + '.png';
	}
});

var OverMarker = Marker.extend({
	getIconProperties: function() {
		return [
			'img/pin-over.png',
			new google.maps.Size(300, 300),
			new google.maps.Point(0, 0),
			new google.maps.Point(140, 175)
		];
	},
	
	onAttach: function() {
		
	}
});


var limitTextArea = function(elem, max, containerSelector) {
	if (containerSelector) {
		$(containerSelector).html((max - elem.value.length >= 0) ? max - elem.value.length : 0)
	}
	
	if (elem.value.length >= max) {
		elem.value = elem.value.substr(0, max - 1);
	}
}

var checkUsernameAvailabilityTimer = null;
var lastUsername = null;
var checkUsernameAvailability = function() {
	clearTimeout(checkUsernameAvailabilityTimer);
	var responseField = $('#availableUsername');
				
	if (lastUsername != $('#usernameRegistrationForm').val()) {
		if ($('#usernameRegistrationForm').val() != '') {
			responseField.removeClass('tc-red').removeClass('tc-green').addClass('tc-gray');
			responseField.html('Verificando disponibilidade...');
		
			checkUsernameAvailabilityTimer = setTimeout(function() {
				$.ajax({
					url: 'users/availableUsername',
					data: {
						username: $('#usernameRegistrationForm').val()
					},
					dataType: 'json',
					success: function(data) {
						lastUsername = $('#usernameRegistrationForm').val();
					
						if (data.success) {
							responseField.removeClass('tc-gray').removeClass('tc-green').addClass('tc-green');
							responseField.html('Username disponível');
						} else {
							responseField.removeClass('tc-gray').removeClass('tc-green').addClass('tc-red');
							responseField.html('Username indisponível');
						}
					}
				});
			}, 750);
		} else {
			responseField.html('');
		}
	}
}

var log = function(a1) {
	console.log(a1);
}

var openLogin = function(error) {
	if (error == null) {
		error = false;
	}
	
	$.openPopupLayer({
        name: 'mdlLogin',
		parameters: {error:error},
        width: 500,
        url: '/login'
	});
}

var adjustLeftColumn = function() {
	$('#boxLeft').height($('#inner').height() - 150 + 'px');
}

var adjustInnerSize = function() {
	var offset = ($.browser.msie && parseInt($.browser.version) == 8) ? 4 : 0; //TODO
	$('#inner').height($(document).height() - offset + 'px');
}

var openModal = function(name) {
	$.openPopupLayer({
        name: name,
        width: 670,
        url: "/"+name
	});
}

$(document).ready(function() {
	adjustInnerSize();
	adjustLeftColumn();
});

$(window).resize(function() {
	adjustInnerSize();
	adjustLeftColumn();
});