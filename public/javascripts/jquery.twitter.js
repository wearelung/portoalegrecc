/*
It's a Twitter Search Library implemented using jQuery
This library was implemented based in Twitter Official Search API documentation
http://apiwiki.twitter.com/w/page/22554756/Twitter-Search-API-Method:-search

@pablocantero
http://pablocantero.com
https://github.com/phstc/jquery-twitter/
*/
var Twitter = {	
	tweets: function(){
		var query = '';
		var from = '';
		var to = '';
		/*
		rpp: Optional. The number of tweets to return per page, up to a max of 100
		*/
		var limit = 100;
		/*
		page: Optional. The page number (starting at 1) to return, up to a max of roughly 1500 results
		*/
		var page = 1;
		var searchUrl = 'http://search.twitter.com/search.json?q=';
		/*
		result_type: 
		mixed: In a future release this will become the default value. Include both popular and real time results in the response.
		recent: The current default value. Return only the most recent results in the response.
		popular: Return only the most popular results in the response.
		*/
		var order = 'recent';
		var locale = '';
		var sanitize = function(value){
			if(value) {
				return encodeURIComponent(value);
			}
			return '';
		};
		var publicMethods =  {
			containing: function(_query){
				query = sanitize(_query);
				return this;
			}, 
			and: function(_query){
				query += '%20' + sanitize(_query);
				return this;
			},
			or: function(_query){
				query += '+OR+' + sanitize(_query);
				return this;			
			},
			from: function(_from){
				from = '+from:' + _from.replace('@', '');
				return this;
			},
			limit: function(_limit){
				limit = _limit;
				return this;
			},
			page: function(_page){
				page = _page
				return this;
			},
			to: function(_to){
				to = '+to:' + _to.replace('@', '');
				return this;
			},
			order: function(_order){
				order = _order
				return this;
			},
			locale: function(_locale) {
				locale = '&locale=' + _locale
				return this;
			},
			toQuery: function(){
				return query + from + to + '&rpp=' + limit + '&page=' + page + locale + '&result_type=' + order;
			},
			all: function(succees, error){
				if(error == null){
					error = function(errorMessage){
						alert(errorMessage);
					}
				}
				$.ajax({
				  url: searchUrl + publicMethods.toQuery(),
				  success: function(data){
				  if(data.error){
						error(data.error, publicMethods);
						return;
					}
					succees(data);
				  },
				  error: function(request, textStatus, errorThrown){
					error(textStatus, publicMethods);
				  },
				  dataType: 'jsonp',
				  async: false
				});
			}
		};
		return publicMethods;
	}
}