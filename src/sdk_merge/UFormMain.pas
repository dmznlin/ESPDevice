unit UFormMain;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, cxGraphics, cxControls, cxLookAndFeels,
  cxLookAndFeelPainters, cxGeometry, dxFramedControl, cxSplitter, uSynEdit,
  uSynMemo, dxPanel, Vcl.ComCtrls, Vcl.ToolWin, Vcl.Menus, Vcl.StdCtrls,
  cxButtons, cxContainer, cxEdit, cxProgressBar;

type
  TfFormMain = class(TForm)
    PanelTop: TdxPanel;
    MemoSrc2: TUSynMemo;
    MemoSrc1: TUSynMemo;
    cxSplitter1: TcxSplitter;
    cxSplitter2: TcxSplitter;
    MemoDest: TUSynMemo;
    dxPanel2: TdxPanel;
    BtnMerge: TcxButton;
    Bar1: TcxProgressBar;
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormResize(Sender: TObject);
    procedure cxSplitter1Moved(Sender: TObject);
    procedure cxSplitter1BeforeClose(Sender: TObject; var AllowClose: Boolean);
    procedure MemoSrc1DropFiles(Sender: TObject; X, Y: Integer;
      AFiles: TStrings);
    procedure BtnMergeClick(Sender: TObject);
  private
    { Private declarations }
    FWidthForm: Integer;
    FWidthMemo: Integer;
    FListDiff: TStrings;
  public
    { Public declarations }
    procedure ParseHas(const nLineA,nLineB: TStrings; CheckVal: Boolean);
  end;

var
  fFormMain: TfFormMain;

implementation

{$R *.dfm}
uses ULibFun;

const
  cPrefix = '#define ';
  cPreLen = Length(cPrefix);

procedure TfFormMain.FormCreate(Sender: TObject);
var nIdx: Integer;
begin
  for nIdx := 0 to ComponentCount - 1 do
   if Components[nIdx] is TUSynMemo then
    (Components[nIdx] as TUSynMemo).Clear;
  //xxxxx

  FWidthForm := 0;
  FListDiff := TStringList.Create;
  TApplicationHelper.LoadFormConfig(self);
end;

procedure TfFormMain.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  TApplicationHelper.SaveFormConfig(self);
  FListDiff.Free;
end;

procedure TfFormMain.FormResize(Sender: TObject);
begin
  if FWidthForm = 0 then
  begin
    FWidthForm := ClientWidth;
    FWidthMemo := Round(PanelTop.ClientWidth / 2) - 10;
    MemoSrc1.Width := FWidthMemo;
  end else
  begin
    MemoSrc1.Width := FWidthMemo + Round((ClientWidth - FWidthForm) / 2);
  end;

  BtnMerge.Left := Round((BtnMerge.Parent.ClientWidth - BtnMerge.Width) / 2);
end;

procedure TfFormMain.MemoSrc1DropFiles(Sender: TObject; X, Y: Integer;
  AFiles: TStrings);
begin
  if (AFiles.Count < 0) or (not FileExists(AFiles[0])) then Exit;
  //invalid

  (Sender as TUSynMemo).Lines.LoadFromFile(AFiles[0]);
end;

procedure TfFormMain.cxSplitter1Moved(Sender: TObject);
begin
  FWidthMemo := MemoSrc1.Width;
end;

procedure TfFormMain.cxSplitter1BeforeClose(Sender: TObject;
  var AllowClose: Boolean);
begin
  AllowClose := False;
end;

procedure TfFormMain.BtnMergeClick(Sender: TObject);
begin
  Bar1.Visible := True;
  try
    FListDiff.Clear;
    MemoDest.Clear;
    MemoDest.Lines.Add('//1,有;2,无');
    ParseHas(MemoSrc1.Lines, MemoSrc2.Lines, True);

    MemoDest.Lines.Add('');
    MemoDest.Lines.Add('//' + StringOfChar('-', 80));
    MemoDest.Lines.Add('//1,无;2,有');
    ParseHas(MemoSrc2.Lines, MemoSrc1.Lines, False);

    MemoDest.Lines.Add('');
    MemoDest.Lines.Add('//' + StringOfChar('-', 80));
    MemoDest.Lines.Add('//1、2有,但不同');
    MemoDest.Lines.AddStrings(FListDiff);
  finally
    Bar1.Visible := False;
  end;
end;

//Date: 2025-04-01
//Parm: #define CONFIG_XX 1
//Desc: 将 CONFIG_XX 或 1 提取出来
function ParseVal(const nVal: string; Key: Boolean = True): string;inline;
var nIdx: Integer;
begin
  nIdx := Pos(cPrefix, LowerCase(nVal));
  if nIdx < 1 then
  begin
    Result := '';
    Exit;
  end;

  Result := Copy(nVal, cPreLen + 1, MaxInt);
  nIdx := Pos(' ', Result);
  if nIdx > 0 then
  begin
    if Key then
         Result := Copy(Result, 1, nIdx - 1)
    else Result := Trim(Copy(Result, nIdx + 1, MaxInt));
  end else
  begin
    Result := '';
  end;
end;

//Date: 2025-04-01
//Parm: 列表
//Desc: 检索nLineA有,但nLineB没有的项
procedure TfFormMain.ParseHas(const nLineA, nLineB: TStrings; CheckVal: Boolean);
var nKey,nVal,nStr: string;
    i,nIdx: Integer;
begin
  Bar1.Properties.Max := nLineA.Count;
  for nIdx := 0 to nLineA.Count - 1 do
  begin
    Bar1.Position := nIdx + 1;
    Application.ProcessMessages;

    nKey := ParseVal(nLineA[nIdx]);
    if nKey = '' then Continue;

    if CheckVal then
      nVal := ParseVal(nLineA[nIdx], False);
    //xxxxx

    for i := 0 to nLineB.Count - 1 do
    begin
      if nKey = ParseVal(nLineB[i]) then
      begin
        if CheckVal then
        begin
          nStr := ParseVal(nLineB[i], False);
          if nVal <> nStr then
            FListDiff.Add(nLineA[nIdx] + ' //' + nStr);
          //xxxxx
        end;

        nKey := '';
        break;
      end;
    end;

    if nKey <> '' then
      MemoDest.Lines.Add(nLineA[nIdx]);
    //xxxxx
  end;
end;

end.
