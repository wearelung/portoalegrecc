require 'extend_string'

class CausesController < ApplicationController
  skip_before_filter :verify_authenticity_token, :only => [ :upload_image ]
  before_filter :remove_fb_comment_id, :only => [:show]
  #  uses_tiny_mce(:options => AppConfig.default_mce_options, :only => [:new, :edit])
  
  def remove_fb_comment_id
    if(params[:fb_comment_id])
      redirect_to show_cause_path(:category => params[:category], :title => params[:title], :id => params[:id])#cause.url#show_cause_path(params[:category], params[:title], params[:id])
    end   
  end
  
  def search_causes
    cookies[:category] = nil unless params[:page]
    params[:page] = 1 if params[:first_page]
    @categories = Category.find(:all, :order => 'name ASC')  
    @causes = Cause.search(params[:search], cookies[:category]).paginate :page => params[:page], :per_page => 3
    respond_to do |format|
      format.html
      format.js { render :pagination }
    end
  end
  
  def show
    @cause = Cause.find(params[:id])
    @cause.update_attribute(:views, @cause.views += 1)
    # @falapoa = @cause.get_falapoa_data unless @cause.protocol == '-1'
    respond_to do |format|
      if (!@cause.is_rejected) and (@cause.submited)
        format.html 
      else
        format.html { redirect_to root_path}
      end
    end
  end
  
  def visibles
    # Pegar os limites do mapa
    currentZoom = params['currentZoom']
    maxZoom = params['maxZoom']
    categories = params['cats']
    positions = {:latA =>params['topLeftY'], :latB =>params['bottomRightY'],:lngA =>params['topLeftX'],:lngB =>params['bottomRightX']}    	        	
    @causes = Cause.find_causes_by_latitude_and_longitude(positions, categories)
    @res = Array.new
    @causes.each do |cause|
      cause['category_name'] = cause.category.name
      cause['url'] = cause.url
      @res << cause
    end
    render :json => @res.to_json
  end
  
  
  def new
    @categories = Category.find(:all, :order => 'name ASC')
    @user = session[:user]
    @cause = Cause.new
    
    @cause.local = params['local']
    @cause.title = "Título vai aqui"
    @cause.abstract = "Conteúdo vai aqui"
    @cause.email = @user.email
    @cause.author = @user.name
    @cause.user_id = @user.id
    @cause.latitude = params['lat']
    @cause.longitude = params['lng']
    @cause.district = params['district']
    @cause.category_id = 1
    
    @cause.save
    respond_to do |format|
      format.html
    end
  end
  
  def create
    @cause = Cause.find params[:id]
    @cause.update_attributes(
        :local => params[:local], 
        :phone_no => params[:phone_no],
        :cell_phone_no => params[:cell_phone_no],
        :category_id => params[:category],
        :title => params[:title],
        :district => params[:district],
        :abstract => params[:abstract],
        :tag_list => params[:tags].gsub(/;/, ','),
        :latitude => params['latLng'][0],
        :longitude => params['latLng'][1],
        :submited => 1
    )
    #@cause.save
    @result = { :status => :ok, :success => true, :data => { :url => @cause.url, :id => @cause.id }}
    respond_to do |format|
      format.json { render :json => @result.to_json }
    end
  end
  
  def check_video_cause
    @rc = RichContent.new
    @rc.kind = 2
    @rc.url = params["url"]
    @rc["success"] = @rc.valid?
    if @rc.valid?
      @rc.update_video_id_and_type
    end
    render :json => @rc.to_json
  end
  
  def add_video_cause
    @rc = RichContent.new
    @rc.kind = 2
    @rc.url = params["url"]
    @rc.cause_id = params["cause"]
    if @rc.valid?
      @rc.update_video_id_and_type
      @rc.save
    end
    render :json => @rc.to_json
  end
  
  def remove_video_cause
    @rc = RichContent.find params["id"]
    @rc.destroy
    render :json => @rc.to_json
  end
  
  def add_url_image
    @rc = RichContent.new
    @rc.cause_id = params[:cause_id]
    @rc.kind = 1
    @rc.url = params[:url]
    @rc['success'] = true
    @rc.save
    render :json => @rc.to_json
  end
  
  def upload_image
    if params['qqfile'].instance_of? String
      @filename = params['qqfile']
      newf = File.open('/tmp/' + @filename, "wb")
      str =  request.body.read
      newf.write(str)
      newf.close
      photo = File.open('/tmp/' + @filename, 'r')
    else
      photo = params['qqfile']
    end
    @rc = RichContent.new
    @rc.photo = photo
    @rc.cause_id = params[:cause_id]
    @rc.kind = 1
    if @rc.save
      @rc['photo_url'] = @rc.photo.url
      @rc['success'] = true
    else
      @rc['success'] = false
    end
    render :json => @rc.to_json
  end
  
  def manage_image_cause
    @rc = RichContent.find(params[:id])
    @rc.manage_image_deletion
    render :json => { :succes => true }
  end
  
  def edit
    @categories = Category.find(:all, :order => 'name ASC')    
    @cause = Cause.find(params[:id])
    @user = session[:user]
    respond_to do |format|
      if @cause.user == session[:user]
        format.html
      else  
        redirect_to root_path
      end
    end
  end
  
  def update
    @cause = Cause.find(params[:id])    
    @cause.local = params[:local]
    @cause.email = params[:email]
    @cause.phone_no = params[:phone_no]
    @cause.cell_phone_no = params[:cell_phone_no]
    @cause.category_id = params[:category]
    @cause.title = params[:title]
    @cause.author = params[:author]
    @cause.district = params[:district]
    @cause.abstract = params[:abstract]
    @cause.tag_list = params[:tags].gsub(/;/, ',')  
    @cause.save
    @result = { :status => :ok, :success => true, :data => { :url => @cause.url }}
    respond_to do |format|
      format.json { render :json => @result.to_json }
    end
  end
  
  def post_comment
    # cause = Cause.find(params[:id])
    # CausesMailer.deliver_send_comment_notification(cause)
    render :nothing => true    
  end

  def optin
    @cause = Cause.find params[:id]
  end

  def send_to_falapoa
    @cause = Cause.find(params[:id])
    @cause.send_to_falapoa(session[:user], show_cause_url(:category => @cause.category, :title =>@cause.title, :id =>@cause.id), params[:reference])
    @result = { :success => true }
    respond_to do |format|
      format.json { render :json => @result.to_json }
    end
  end
  
end
