# Methods added to this helper will be available to all templates in the application.
module ApplicationHelper
  # for jquery
  def csrf_meta_tag
    if protect_against_forgery?
      out = %(<meta name="csrf-param" content="%s"/>\n)
      out << %(<meta name="csrf-token" content="%s"/>)
      out % [ Rack::Utils.escape_html(request_forgery_protection_token),
      Rack::Utils.escape_html(form_authenticity_token) ]
    end
  end
  
  # get user image
  def user_image(user, width = 56, height = 56)
    image_url = user.profile_image_url.blank? ? "/img/avatar.jpg" : user.profile_image_url
    image_src =  %{ <img id="image" src="#{image_url}" width="#{width}" height="#{height}" class="fl" alt="#{user.name}" /> }
    if user.facebook_id
      profile_url = %{ http://www.facebook.com/profile.php?id=#{user.facebook_id}}
    elsif user.twitter_username
      profile_url = %{ http://twitter.com/#!/#{user.twitter_username} }
    end
    if profile_url 
      link_to image_src, profile_url, :target => "_blank"
    else
      image_src
    end
  end
  
  def logged_user?
    # usuario esta na sessão, pertence a classe usuário e não é do twitter faltando digitar email
    !session[:user].blank? && session[:user].class == User && !session['collect_email']
  end
  
  def current_user
    if logged_user?
      session[:user]
    else
      nil
    end
  end
  
  def snippet(thought) 
    wordcount = 10 
    thought.split[0..(wordcount-1)].join(" ") + (thought.split.size > wordcount ? "..." : "") 
  end 
end
