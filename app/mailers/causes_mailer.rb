class CausesMailer < BaseMailer
  
  def send_comment_notification(cause)
    defaults
    subject "PortoAlegre.cc - notificação de comentário em causa"
    if cause.email.nil?
      recipients ['contato@portoalegre.cc']
    else
      recipients [cause.email,'contato@portoalegre.cc']
    end
    body :cause => cause
  end
  
end