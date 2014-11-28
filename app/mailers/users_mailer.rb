class UsersMailer < BaseMailer
  
  def registro(user)
    defaults
    subject "Seja bem vindo ao portoalegre.cc"
    content_type "text/html"
    recipients user.email
    body :user => user
  end
  
end