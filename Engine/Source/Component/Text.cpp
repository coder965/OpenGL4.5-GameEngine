//#include <pch.h>
#include "Text.h"

#include <freetype-gl/freetype-gl.h>
#include <freetype-gl/text-buffer.h>
#include <freetype-gl/texture-font.h>
#include <freetype-gl/vertex-buffer.h>
#include <freetype-gl/edtaa3func.h>
#include <freetype-gl/vec234.h>

#include <FreeType/ft2build.h>
#include FT_FREETYPE_H

#include <Component/Transform/Transform.h>
#include <Component/ObjectInput.h>
#include <Component/Renderer.h>
#include <Component/AABB.h>
#include <Component/Mesh.h>

#include <Manager/Manager.h>
#include <Manager/FontManager.h>

#include <GPU/Shader.h>

#include <Utils/GPU.h>


Text::Text()
{
	offset = new Transform();
	transform->SetScale(glm::vec3(0.005f));

	atlasTextureID = Manager::Font->atlas->id;
	color = glm::vec3(0.967f, 0.333f, 0.486f);

	mesh = new Mesh();
	mesh->UseMaterials(false);
}

Text::~Text() {
}

const char * Text::GetText() const
{
	return content.c_str();
}

void Text::SetColor(glm::vec3 & color)
{
	this->color = color;
}

void Text::SetText(const string &text)
{
	if (content.compare(text) == 0)
		return;
	content.assign(text);
	if (content.length())
		InitText();
}

void Text::AppendText(const char * text)
{
	SetText((content + text).c_str());
}

void Text::Render(const Shader *shader) const
{
	if (content.length() == 0)
		return;

	renderer->Use();

	glUniform3f(shader->text_color, color.r, color.g, color.b);
	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(transform->GetModel()));

	// TODO - move texture to Material Mesh
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlasTextureID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mesh->Render(shader);

	glBindVertexArray(0);
	glDisable(GL_BLEND);
	CheckOpenGLError();
}


void Text::InitText()
{
	texture_font_t* font = Manager::Font->font;

	size_t i;
	unsigned int index = 0;
	vec2 cue = {0, 0};
	unsigned int len = content.length();

	mesh->ClearData();

	for(i=0; i<len; i++)
	{
		texture_glyph_t *glyph = texture_font_get_glyph( font, content[i] );
		if( glyph != NULL )
		{
			float kerning = 0.0f;
			if( i > 0) {
				kerning = texture_glyph_get_kerning(glyph, content[i-1]);
			}
			cue.x += kerning;
			int x0  = (int)( cue.x + glyph->offset_x );
			int y0  = (int)( cue.y + glyph->offset_y );
			int x1  = (int)( x0 + glyph->width );
			int y1  = (int)( y0 - glyph->height );
			float s0 = glyph->s0;
			float t0 = glyph->t0;
			float s1 = glyph->s1;
			float t1 = glyph->t1;

			mesh->indices.push_back( index );
			mesh->indices.push_back( index + 1);
			mesh->indices.push_back( index + 2);

			mesh->indices.push_back( index );
			mesh->indices.push_back( index + 2);
			mesh->indices.push_back( index + 3);
			index += 4;

			mesh->positions.push_back(glm::vec3(x0, y0, 0));
			mesh->positions.push_back(glm::vec3(x0, y1, 0));
			mesh->positions.push_back(glm::vec3(x1, y1, 0));
			mesh->positions.push_back(glm::vec3(x1, y0, 0));

			mesh->normals.push_back(glm::vec3(0, 0, 1));
			mesh->normals.push_back(glm::vec3(0, 0, 1));
			mesh->normals.push_back(glm::vec3(0, 0, 1));
			mesh->normals.push_back(glm::vec3(0, 0, 1));

			mesh->texCoords.push_back(glm::vec2(s0, t0));
			mesh->texCoords.push_back(glm::vec2(s0, t1));
			mesh->texCoords.push_back(glm::vec2(s1, t1));
			mesh->texCoords.push_back(glm::vec2(s1, t0));

			cue.x += glyph->advance_x;
		}
	}

	mesh->InitFromData();
	SetupAABB();
}