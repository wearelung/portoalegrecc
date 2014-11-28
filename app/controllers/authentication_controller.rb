class AuthenticationController < ApplicationController
  def new
    @user = User.new
  end

  def create
    @user = User.find_by_username params[:username]
    @data = Hash.new
    @data['invalid'] = @user.blank? ? true : false
    if @user
      session[:us] = @user
      session[:type] = "login"
      @data['twitter'] = @user.twitter_user_id.blank? ? false : true
      @data['facebook'] = @user.facebook_id.blank? ? false : true
      @data['google'] = @user.google_email.blank? ? false : true
    end
    render :json => @data.to_json
  end

  def destroy
    flash[:notice] = "Deslogado com sucesso"
    session['collect_email'] = nil
    session[:user] = nil
    if request.referer.nil?
      redirect_to root_path
    else
      redirect_to request.referer
    end
  end

  def failure
    flash[:notice] = "Sua permissão é necessária para ter acesso ao sistema"
    session[:user] = nil
    session['error'] = 'register'
    redirect_to root_path
  end

  def blank
    render :text => "Not Found", :status => 404
  end

end
