ActionController::Routing::Routes.draw do |map|
  map.update_twitter_email '/users/update_twitter_email', :controller => 'users', :action => 'update_twitter_email'
  map.resources :users
  map.root :controller => :application, :action => :index
  
  map.login '/login', :controller => 'authentication', :action => 'new'
  map.logout '/logout', :controller => 'authentication', :action => 'destroy'

  map.failure '/auth/failure', :controller => 'authentication', :action => 'failure'
  map.callback '/auth/:provider', :controller => 'authentication', :action => 'blank'
  map.callback '/auth/:provider/callback', :controller => 'users', :action => 'create'
  
  map.visibles_causes '/causes/visibles', :controller => 'causes', :action => 'visibles'
  map.optin_causes '/causas/optin/:id', :controller => 'causes', :action => 'optin'
  map.send_to_falapoa '/causas/send_to_falapoa/:id', :controller => 'causes', :action => 'send_to_falapoa'
  
  map.show_cause '/causas/:category/:title/:id', :controller => 'causes', :action => 'show'
  map.connect '/site/causas/:category/:title/:id', :controller => 'causes', :action => 'show'

  map.new_cause '/causas/new', :controller => 'causes', :action => 'new'
  map.create_cause '/causas/create', :controller => 'causes', :action => 'create'
  map.check_video_cause '/causas/check_video_cause', :controller => 'causes', :action => 'check_video_cause'
  map.add_video_cause '/causas/add_video_cause', :controller => 'causes', :action => 'add_video_cause'
  map.remove_video_cause '/causas/remove_video_cause', :controller => 'causes', :action => 'remove_video_cause'
  map.edit_cause '/causes/edit/:id', :controller => 'causes', :action => 'edit'
  map.update_cause '/causes/update/:id', :controller => 'causes', :action => 'update'
  map.post_comment '/post_comment/:id', :controller => 'causes', :action => 'post_comment'
  map.search_causes '/search_causes', :controller => 'causes', :action => 'search_causes'
  map.upload_image '/upload_image', :controller => 'causes', :action => 'upload_image'
  map.manage_image_cause '/causes/manage_image', :controller => 'causes', :action => 'manage_image_cause'
  map.add_url_image '/causes/add_url_image', :controller => 'causes', :action => 'add_url_image'

  # map.resources 'cause'
  
  map.sobre_o_projeto '/sobre_o_projeto', :controller => 'application', :action => 'sobre_o_projeto'
  map.apoiadores '/apoiadores', :controller => 'application', :action => 'apoiadores'
  map.seja_um_voluntario '/seja_um_voluntario', :controller => 'application', :action => 'seja_um_voluntario'
  map.fale_conosco '/fale_conosco', :controller => 'application', :action => 'fale_conosco'
  map.termos_de_uso '/termos_de_uso', :controller => 'application', :action => 'termos_de_uso'
  map.como_participar '/como_participar', :controller => 'application', :action => 'como_participar'
  map.send_contact_form '/send_contact_form', :controller => 'application', :action => 'send_contact_form'
  map.send_volunteer_form '/send_volunteer_form', :controller => 'application', :action => 'send_volunteer_form'
  
  #admin
  map.show_causes '/admin/causes', :controller => 'admin', :action => 'show_causes'
  map.show_users_list '/admin/users', :controller => 'admin', :action => 'show_users_list'
  map.show_rejected_causes '/admin/rejected_causes', :controller =>'admin', :action => 'show_rejected_causes'
  map.delete_cause '/admin/delete_cause', :controller => 'admin', :action => 'delete_cause'
  map.accept_cause '/admin/accept_cause', :controller => 'admin', :action => 'accpet_cause'
  map.reject_cause '/admin/reject_cause', :controller => 'admin', :action => 'reject_cause'
  map.update_cause_likes '/admin/update_cause_likes', :controller => 'admin', :action => 'update_cause_likes'
  map.admin_index '/admin/', :controller => 'admin', :action => 'index'
  map.admin_login '/admin/login', :controller => 'admin', :action => 'login', :via => :post
  map.admin_logout '/admin/logout', :controller => 'admin', :action => 'logout'
  
  map.connect ':controller/:action/:id'
  map.connect ':controller/:action/:id.:format'
  
end
