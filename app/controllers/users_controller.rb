class UsersController < ApplicationController
  def new
    @user = User.find_by_username(params[:username])
    @data = Hash.new
    @data['username'] = params[:username]
    @data['success'] = @user == nil
    unless @user
      session[:user] = params[:username]
      session[:type] = "signup"
    end
    render :json => @data.to_json
  end
  

  def create
    type = session[:type]
    omniauth = request.env["omniauth.auth"]
    if type == "signup"
      @user = User.new(:username => session[:user])
      if omniauth['provider'] == 'twitter'
        @user['name'] = omniauth['info']['name']
        @user['twitter_user_id'] = omniauth['uid']
        @user['profile_image_url'] = omniauth['info']['image']
        @user['location'] = omniauth['info']['location']
        @user['twitter_username'] = omniauth['extra']['raw_info']['screen_name']
        @user['last_sign_in'] = Time.now
        if User.find_by_twitter_user_id @user['twitter_user_id']
          flash[:notice] = I18n.t "user.notices.twitter_already_registered"
          session['error'] = 'register'
          session[:user] = nil
          session[:us] = nil
        else
          session['collect_email'] = true
          session[:us] = @user
        end
        if request.referer.nil?
          redirect_to root_path
        else
          redirect_to request.referer
        end
      elsif omniauth['provider'] == 'google'
        @user['name'] = omniauth['info']['name']
        @user['google_email'] = omniauth['info']['email']
        @user['email'] = @user['google_email']
        @user['last_sign_in'] = Time.now
        if User.find_by_google_email @user['google_email']
          flash[:notice] = I18n.t "user.notices.google_email_already_registered"
          session['error'] = 'register'
          session[:user] = nil
        else
          session[:user] = @user
          @user.save
        end
        if request.referer.nil?
          redirect_to root_path
        else
          redirect_to request.referer
        end
      elsif omniauth['provider'] == 'facebook'
        @user['name'] = omniauth['info']['name']
        @user['facebook_id'] = omniauth['uid']
        @user['profile_image_url'] = omniauth['info']['image']
        @user['location'] = omniauth['info']['location']
        @user['email'] = omniauth['info']['email']
        @user['last_sign_in'] = Time.now
        if User.find_by_facebook_id @user['facebook_id']
          flash[:notice] = I18n.t "user.notices.facebook_already_registered"
          session['error'] = 'register'
          session[:user] = nil
        else
          session[:user] = @user
          @user.save
        end
        if request.referer.nil?
          redirect_to root_path
        else
          redirect_to request.referer
        end
      end
    elsif type == "login"
      @user = session[:us]
      if omniauth['provider'] == 'twitter'
        if @user['twitter_user_id'].to_s == omniauth['uid']
          #tem que chamar aquele prompt para atualizar o email caso seja vazio!
          session['collect_email'] = true if @user['email'].nil?
          @user.update_attribute(:twitter_username, omniauth['extra']['raw_info']['screen_name'])
          session[:user] = session[:us] 
        else
          flash[:notice] = I18n.t "user.notices.twitter_incorrect_associated"
          session['error'] = 'login'
          session[:user] = nil
        end
      elsif omniauth['provider'] == 'google'
        if @user['google_email'] == omniauth['info']['email']
          @user.update_attribute(:email,omniauth['info']['email']) if @user['email'].nil?
          session[:user] = session[:us]
        else
          flash[:notice] = I18n.t "user.notices.google_email_incorrect_associated"
          session['error'] = 'login'
          session[:user] = nil
        end
      elsif omniauth['provider'] == 'facebook'
        if @user['facebook_id'] == omniauth['uid']
          @user.update_attribute(:email,omniauth['info']['email']) if @user['email'].nil?
          session[:user] = session[:us]
        else
          flash[:notice] = I18n.t "user.notices.facebook_incorrect_associated"
          session['error'] = 'login'
          session[:user] = nil
        end
      end
      if session['error'].nil?
        @user.update_attribute(:last_sign_in,Time.now)
      end
      if request.referer.nil?
        redirect_to root_path
      else
        redirect_to request.referer
      end
    end
  end
  
  def update_twitter_email
    @user = session[:us]
    @user['email'] = params['email']
    @data = Hash.new
    @data['success'] = @user.valid?
    if @user.valid?
      @user.save
      session['collect_email'] = nil
      session[:user] = @user
    end
    render :json => @data.to_json
  end
  
  #TODO: Remover no futuro
  def index
    @user = session[:user]
  end
end
