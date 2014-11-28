class AdminController < ApplicationController

  before_filter :authorize, :except => [:index, :login]
  helper_method :current_user, :cause_sort_column, :sort_direction, :user_sort_column
  
  def index
    if session[:user_id]
      redirect_to :action => :show_causes
    end
  end
  
  def login
    user = User.authenticate(params[:username], params[:password])
    if user
      session[:user_id] = user.id
      flash[:message] = "Login efetuado com sucesso"
      redirect_to :action => :show_causes
    else
      flash[:message] = "Usuário ou senha incorretos"
      redirect_to :action => :index
    end
  end
  
  def logout
    session[:user_id] = nil
    flash[:message] = "Saiu com sucesso"
    redirect_to :action => :index
  end
  
  def show_causes
    params[:category] = "0" if params[:category].blank?
    where = '(causes.id > 0) and (causes.submited = 1)'
    where << " and (causes.author like '%#{params[:author]}%')" unless params[:author].blank? 
    where << " and (causes.title like '%#{params[:title]}%')" unless params[:title].blank?
    where << " and (causes.abstract like '%#{params[:abstract]}%')" unless params[:abstract].blank?
    where << " and (causes.category_id = '#{params[:category]}')" unless params[:category] == "0"
    @causes2 = Admin.get_causes(where)
    @causes = Cause.paginate :all, :conditions => where, :joins => :category, :page => params[:page], :per_page => 10, :order => "#{cause_sort_column} #{sort_direction}"
    @categories = [Category.new(:id => 0, :name => 'Todos')] + Category.find(:all)
  end
  
  def show_users_list
    @twitter_users = Admin.get_total_twitter_users
    @facebook_users = Admin.get_total_facebook_users
    @gmail_users = Admin.get_total_gmail_users
    @users2 = Admin.get_users
    @users = User.paginate(:all,:page => params[:page], :per_page => 10, :order => "#{user_sort_column} #{sort_direction}")
  end
  
  def show_rejected_causes
    params[:category] = "0" if params[:category].blank?
    where = '(causes.is_rejected = 1) and (causes.submited = 1)'
    where << " and (causes.author like '%#{params[:author]}%')" unless params[:author].blank? 
    where << " and (causes.title like '%#{params[:title]}%')" unless params[:title].blank?
    where << " and (causes.abstract like '%#{params[:abstract]}%')" unless params[:abstract].blank?
    where << " and (causes.category_id = '#{params[:category]}')" unless params[:category] == "0"
    @causes2 = Admin.get_rejected_causes(where)
    @causes = Cause.paginate :all, :joins => :category, :conditions => where, :page => params[:page], :per_page => 10, :order => "#{cause_sort_column} #{sort_direction}"
    @categories = [Category.new(:id => 0, :name => 'Todos')] + Category.find(:all)
  end
  
  def delete_cause
    @cause = Cause.find(params[:id])
    @cause.destroy
    redirect_to request.referer
  end
  
  def accept_cause
    @cause = Cause.find(params[:id])
    @cause.update_attributes :is_rejected => 0
    redirect_to request.referer
  end
  
  def reject_cause
    @cause = Cause.find(params[:id])
    @cause.update_attributes :is_rejected => 1
    redirect_to request.referer
  end

  def update_cause_likes
    @cause = Cause.find(params[:id])
    url = "http://www.portoalegre.cc/causas/#{@cause.category.name.urlize}/#{@cause.title.urlize}/#{@cause.id}"
    response = ActiveSupport::JSON.decode(RestClient.get("https://graph.facebook.com/#{url}"))
    @cause.update_attributes :likes => response["shares"].nil? ? 0 : response["shares"], :last_likes_update => Time.now
    redirect_to request.referer
  end
  
  private  
  def current_user
    @current_user ||= User.find(session[:user_id]) if session[:user_id]
  end
  
  def authorize
    if session[:user_id]
        true
    else
      flash[:message] = "Você não tem autorização para acessar este local."  
      redirect_to :action => :index
      false  
    end 
  end
  
  def cause_sort_column
    %w[categories.name  title abstract local district author created_at views likes].include?(params[:sort]) ? params[:sort] : "created_at"
  end

  def user_sort_column
    %w[username  name last_sign_in location twitter_user_id google_email facebook_id].include?(params[:sort]) ? params[:sort] : "last_sign_in"
  end
  
  def sort_direction
    %w[asc desc].include?(params[:direction]) ? params[:direction] : "desc"
  end
  
end
