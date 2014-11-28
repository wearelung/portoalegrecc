class ContactMailer < BaseMailer

  def send_contact_form(form_data)
    defaults
    subject "Fale conosco - Envio através do site"
    recipients ["contato@portoalegre.cc"]
    body :data => form_data
  end  

end