program merger;

uses
  Vcl.Forms,
  UFormMain in 'UFormMain.pas' {Form1};

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TfFormMain, fFormMain);
  Application.Run;
end.
